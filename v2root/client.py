"""Professional high-level client built on the native ABI wrapper."""

import json
import os
import threading
from pathlib import Path

from .core import CoreManager
from .errors import ConfigurationError, NativeCallError
from .models import LatencyResult, RealtimeSpeed, RuntimeStatus, Traffic, VersionInfo
from .native import NativeCore


class V2RootClient:
    """Primary SDK facade.

    Construction resolves or installs a compatible core. Configuration methods
    accept either share URIs, raw JSON, or configuration file paths supported
    by V2Root Core. Lifecycle operations are serialized per client.
    """
    def __init__(
        self,
        http_port=2300,
        socks_port=2301,
        *,
        core_path=None,
        core_manager=None,
        auto_install=True,
        geosite_path=None,
        native_factory=NativeCore,
    ):
        """Initialize the SDK, resolve a core binary, and bind its native ABI.

        Args:
            http_port (int): Local HTTP proxy port in the range ``1..65535``.
            socks_port (int): Local SOCKS proxy port in the range ``1..65535``.
            core_path (str | os.PathLike[str] | None): Explicit DLL or SO path.
                When omitted, ``core_manager`` resolves or installs a core.
            core_manager (CoreManager | None): Manager used for automatic core
                discovery and installation.
            auto_install (bool): Allow downloading the latest compatible core
                when no active installation exists.
            geosite_path (str | os.PathLike[str] | None): Optional geosite asset
                path forwarded to parser and runtime operations.
            native_factory (Callable[[os.PathLike[str]], NativeCore]): Factory
                used to construct the native ABI wrapper.

        Raises:
            TypeError: If either proxy port is not an integer.
            ValueError: If ports are invalid or equal.
            CoreNotInstalledError: If no core exists and installation is disabled.
            CoreError: If automatic installation fails.
            CoreCompatibilityError: If the selected binary cannot be loaded.
        """
        self._validate_port("http_port", http_port)
        self._validate_port("socks_port", socks_port)
        if http_port == socks_port:
            raise ValueError("http_port and socks_port must be different")
        self.http_port = http_port
        self.socks_port = socks_port
        self.geosite_path = os.fspath(geosite_path) if geosite_path else None
        self.core_manager = core_manager or CoreManager()
        if core_path is None:
            core_path = self.core_manager.ensure(auto_install=auto_install).library_path
        self.native = native_factory(core_path)
        self.library_path = Path(core_path).expanduser().resolve()
        self._config = None
        self._lifecycle_lock = threading.RLock()

    @staticmethod
    def _validate_port(name, value):
        """Validate one local proxy port.

        Args:
            name (str): Parameter name used in error messages.
            value (object): Candidate port value.

        Returns:
            None: Successful validation has no return value.

        Raises:
            TypeError: If ``value`` is not an integer.
            ValueError: If ``value`` is outside ``1..65535``.
        """
        if not isinstance(value, int):
            raise TypeError(f"{name} must be an integer")
        if not 1 <= value <= 65535:
            raise ValueError(f"{name} must be between 1 and 65535")

    def _options(self, extra=None):
        """Serialize options shared by parser and runtime operations.

        Args:
            extra (dict[str, object] | None): Additional options that override
                defaults such as ``uri`` or routing settings.

        Returns:
            str: Compact UTF-8-safe JSON text accepted by V2Root Core.
        """
        value = {"httpPort": self.http_port, "socksPort": self.socks_port}
        if self.geosite_path:
            value["geositePath"] = self.geosite_path
        if extra:
            value.update(extra)
        return json.dumps(value, ensure_ascii=False, separators=(",", ":"))

    def parse(self, uri, **options):
        """Convert a share URI into normalized Xray-compatible JSON.

        Args:
            uri (str): VLESS, VMess, Trojan, or Shadowsocks share URI.
            **options: Additional parser options documented by V2Root Core.

        Returns:
            str: Complete normalized JSON configuration.

        Raises:
            ValueError: If ``uri`` is empty or not a string.
            ConfigurationError: If the native parser returns an empty result.
        """
        if not isinstance(uri, str) or not uri.strip():
            raise ValueError("uri must be a non-empty string")
        payload = self._options({"uri": uri, **options})
        result = self.native.string_call("Parse", self.native.encode(payload))
        if not result:
            raise ConfigurationError("V2Root Core could not parse the URI")
        return result

    def set_config(self, config_input, parse_uri=True):
        """Normalize, validate, and retain a runtime configuration.

        Args:
            config_input (str): Share URI, raw JSON, or native-supported path.
            parse_uri (bool): Parse recognized share URI schemes before validation.

        Returns:
            str: Normalized configuration retained by this client.

        Raises:
            ValueError: If ``config_input`` is empty or not a string.
            ConfigurationError: If parsing or validation fails.
        """
        if not isinstance(config_input, str) or not config_input.strip():
            raise ValueError("config_input must be a non-empty string")
        schemes = ("vless://", "vmess://", "trojan://", "ss://")
        value = (
            self.parse(config_input)
            if parse_uri and config_input.lower().startswith(schemes)
            else config_input
        )
        self.validate(value)
        self._config = value
        return value

    def validate(self, config_input=None):
        """Validate a configuration without starting a persistent runtime.

        Args:
            config_input (str | None): Configuration to validate. ``None`` uses
                the configuration previously selected by :meth:`set_config`.

        Returns:
            dict[str, object]: Native validation response, normally
                ``{"result": "valid"}``.

        Raises:
            ValueError: If no configuration is available.
            ConfigurationError: If native validation reports an error.
        """
        value = config_input or self._config
        if not value:
            raise ValueError("No configuration has been provided")
        try:
            return self.native.json_call(
                "ValidateConfig",
                self.native.encode(value),
                self.native.encode(self._options()),
            )
        except NativeCallError as exc:
            raise ConfigurationError(str(exc)) from exc

    def start(self, config_input=None, validate=True):
        """Start the process-global embedded runtime.

        Args:
            config_input (str | None): Configuration to start. ``None`` uses
                the retained configuration.
            validate (bool): Validate before startup when true.

        Returns:
            str: Runtime status after startup is accepted.

        Raises:
            ValueError: If no configuration is available.
            ConfigurationError: If pre-start validation fails.
            NativeCallError: If the core rejects startup.
        """
        with self._lifecycle_lock:
            value = config_input or self._config
            if not value:
                raise ValueError("No configuration has been provided")
            if validate:
                self.validate(value)
            error = self.native.string_call(
                "Start",
                self.native.encode(value),
                self.native.encode(self._options()),
            )
            if error is not None:
                raise NativeCallError(error)
            self._config = value
            return self.status

    def stop(self, ignore_stopped=True):
        """Stop the process-global embedded runtime.

        Args:
            ignore_stopped (bool): Treat ``server not running`` as success.

        Returns:
            str: Runtime status after the stop operation.

        Raises:
            NativeCallError: If shutdown fails for another reason.
        """
        with self._lifecycle_lock:
            error = self.native.string_call("Stop")
            if error and not (ignore_stopped and error == "server not running"):
                raise NativeCallError(error)
            return self.status

    @property
    def status(self):
        """Return the raw lifecycle status reported by V2Root Core.

        Returns:
            str: One of ``STOPPED``, ``STARTING``, ``RUNNING``, or ``STOPPING``.
        """
        return self.native.string_call("GetStatus")

    @property
    def runtime_status(self) -> RuntimeStatus:
        """Return the lifecycle status as a typed enumeration.

        Returns:
            RuntimeStatus: Typed representation of the native status string.

        Raises:
            ValueError: If a future core returns an unknown status value.
        """
        return RuntimeStatus(self.status)

    @property
    def running(self) -> bool:
        """Return whether the embedded runtime currently reports ``RUNNING``.

        Returns:
            bool: ``True`` only for :attr:`RuntimeStatus.RUNNING`.
        """
        return self.runtime_status is RuntimeStatus.RUNNING

    @property
    def version_info(self):
        """Return typed build metadata for the loaded native core.

        Returns:
            VersionInfo: Core code version, upstream version, and build date.

        Raises:
            NativeCallError: If native metadata is missing or invalid JSON.
        """
        return VersionInfo.from_dict(self.native.json_call("GetVersionInfo"))

    @property
    def traffic(self):
        """Return cumulative non-API outbound traffic counters.

        Returns:
            Traffic: Uploaded and downloaded byte counts.

        Raises:
            NativeCallError: If the runtime is stopped or native JSON is invalid.
        """
        value = self.native.json_call("GetTotalTraffics")
        return Traffic(int(value["uplink"]), int(value["downlink"]))

    @property
    def speed(self):
        """Return upload and download rates calculated by the native core.

        Returns:
            RealtimeSpeed: Upload and download rates in bytes per second.

        Raises:
            NativeCallError: If the runtime is stopped or native JSON is invalid.
        """
        value = self.native.json_call("GetRealtimeSpeed")
        return RealtimeSpeed(
            float(value["uplinkSpeed"]), float(value["downlinkSpeed"])
        )

    def test_latency(self, configs, test_url="", timeout=10):
        """Measure latency for one or more configurations.

        Args:
            configs (str | os.PathLike[str] | Iterable[str]): One config, a text
                file containing configs, or an ordered iterable.
            test_url (str): HTTP(S) endpoint used by the native latency probe.
            timeout (int): Per-test timeout in seconds.

        Returns:
            list[int | str]: Ordered numeric latencies and native error strings.

        Raises:
            ValueError: If the normalized configuration collection is empty.
            NativeCallError: If the native batch operation itself fails.
        """
        configs = self._normalize_configs(configs)
        if not isinstance(configs, (list, tuple)) or not configs:
            raise ValueError("configs must contain at least one configuration")
        result = self.native.json_call(
            "TestLatency",
            self.native.encode(json.dumps(list(configs), ensure_ascii=False)),
            self.native.encode(test_url),
            timeout,
        )
        return [int(item) if str(item).isdigit() else str(item) for item in result]

    def test_latency_detailed(self, configs, test_url="", timeout=10) -> list[LatencyResult]:
        """Measure latency and return structured per-configuration results.

        Args:
            configs (str | os.PathLike[str] | Iterable[str]): Config source.
            test_url (str): HTTP(S) endpoint used by the native probe.
            timeout (int): Per-test timeout in seconds.

        Returns:
            list[LatencyResult]: Ordered results containing config, latency, and error.

        Raises:
            ValueError: If no configurations are supplied.
            NativeCallError: If the native batch operation fails.
        """
        normalized = self._normalize_configs(configs)
        values = self.test_latency(normalized, test_url=test_url, timeout=timeout)
        return [
            LatencyResult(
                index=index,
                config=config,
                latency_ms=value if isinstance(value, int) else None,
                error=None if isinstance(value, int) else value,
            )
            for index, (config, value) in enumerate(zip(normalized, values))
        ]

    @staticmethod
    def _normalize_configs(configs):
        """Normalize supported config inputs into an independent list.

        Args:
            configs (str | os.PathLike[str] | Iterable[str]): URI, file path, or
                iterable of configuration strings.

        Returns:
            list[str]: Normalized configuration strings.

        Raises:
            TypeError: If ``configs`` is neither path-like nor iterable.
            UnicodeError: If a config file is not valid UTF-8.
        """
        if isinstance(configs, (str, os.PathLike)):
            value = os.fspath(configs)
            if "://" not in value:
                try:
                    path = Path(value)
                    if path.is_file():
                        return [
                            line.strip()
                            for line in path.read_text(encoding="utf-8").splitlines()
                            if line.strip()
                        ]
                except OSError:
                    pass
            return [value]
        return list(configs)

    def connect(self, config_input, *, validate=True):
        """Select a configuration and start the runtime in one operation.

        Args:
            config_input (str): Share URI, JSON, or supported config path.
            validate (bool): Validate again immediately before startup.

        Returns:
            V2RootClient: This client for fluent usage.

        Raises:
            ConfigurationError: If parsing or validation fails.
            NativeCallError: If startup fails.
        """
        self.set_config(config_input)
        self.start(validate=validate)
        return self

    def update_geo_assets(self, asset_path=""):
        """Download geosite and geoip assets through the native core.

        Args:
            asset_path (str | os.PathLike[str]): Destination directory. An empty
                path requests the core default.

        Returns:
            dict[str, str]: Per-file download status returned by the core.

        Raises:
            NativeCallError: If the native response is invalid or globally fails.
        """
        return self.native.json_call(
            "UpdateGeoAssets", self.native.encode(os.fspath(asset_path))
        )

    def to_share_uri(self, config):
        """Convert a JSON configuration to a supported share URI.

        Args:
            config (str | Mapping[str, object]): JSON text or serializable mapping.

        Returns:
            str: VLESS, VMess, Trojan, or Shadowsocks share URI.

        Raises:
            TypeError: If a non-string config is not JSON serializable.
            ConfigurationError: If no outbound can be converted.
        """
        value = config if isinstance(config, str) else json.dumps(config)
        result = self.native.string_call(
            "JSONToConfigString", self.native.encode(value)
        )
        if not result:
            raise ConfigurationError("The configuration cannot be converted")
        return result

    def configure_logging(self, level=None, output=None):
        """Configure native log level and destination.

        Args:
            level (str | None): Native level such as ``warning`` or ``debug``.
            output (str | os.PathLike[str] | None): Writable native log path.

        Returns:
            None: Native process-global logging settings are updated in place.
        """
        if level is not None:
            self.native.void_call("SetLogLevel", self.native.encode(level))
        if output is not None:
            self.native.void_call(
                "SetLogOutput", self.native.encode(os.fspath(output))
            )

    def update_core(self):
        """Update the managed core while the runtime is stopped.

        Returns:
            UpdateResult: Previous/current installations and change status.

        Raises:
            NativeCallError: If the runtime is not stopped.
            CoreError: If release discovery, download, or installation fails.
        """
        if self.status != "STOPPED":
            raise NativeCallError("Stop the runtime before updating V2Root Core")
        return self.core_manager.update()

    def test_subscription(self, manager, subscription_id=None, timeout=10):
        """Test subscription configs and persist their results.

        Args:
            manager (SubscriptionManager): Manager owning configs and persistence.
            subscription_id (str | None): Restrict testing to one source.
            timeout (int): Per-test timeout in seconds.

        Returns:
            list[LatencyResult]: Ordered structured measurements.

        Raises:
            ValueError: If the selected source has no configurations.
            NativeCallError: If the native latency operation fails.
            PersistenceError: If results cannot be saved.
        """
        configs = (
            manager.get_configs_from_subscription(subscription_id)
            if subscription_id
            else manager.get_all_configs()
        )
        results = self.test_latency_detailed(configs or [], timeout=timeout)
        for result in results:
            manager.record_latency(
                result.config,
                result.latency_ms or -1,
                result.succeeded,
            )
        return results

    def __enter__(self):
        """Return this client for context-managed runtime usage.

        Returns:
            V2RootClient: This client instance.
        """
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Stop an active runtime when leaving a context.

        Args:
            exc_type (type[BaseException] | None): Active exception type.
            exc_value (BaseException | None): Active exception value.
            traceback (types.TracebackType | None): Active traceback.

        Returns:
            None: Exceptions from the managed block are not suppressed.
        """
        if self.status != "STOPPED":
            self.stop()
