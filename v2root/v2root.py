"""Backward-compatible facade for the V2Root 2.x client."""

from .client import V2RootClient


class V2ROOT(V2RootClient):
    """Expose familiar method names while executing the V2Root 2.0 implementation.

    New applications should prefer :class:`V2RootClient`. This class exists to
    ease source migration; it does not load the retired C wrapper or launch an
    external V2Ray executable.
    """
    def __init__(
        self,
        http_port=2300,
        socks_port=2301,
        v2ray_path=None,
        lib_path=None,
        geosite_path=None,
        auto_install=True,
        core_manager=None,
    ):
        """Initialize the modern client while accepting legacy argument names.

        Args:
            http_port (int): Local HTTP proxy port.
            socks_port (int): Local SOCKS proxy port.
            v2ray_path (str | None): Deprecated and ignored external executable path.
            lib_path (str | os.PathLike[str] | None): Explicit V2Root Core binary.
            geosite_path (str | os.PathLike[str] | None): Optional geosite asset path.
            auto_install (bool): Download a core automatically when absent.
            core_manager (CoreManager | None): Custom core manager.

        Raises:
            TypeError: If proxy ports have invalid types.
            ValueError: If proxy ports are invalid.
            CoreError: If core resolution or loading fails.
        """
        super().__init__(
            http_port=http_port,
            socks_port=socks_port,
            core_path=lib_path,
            core_manager=core_manager,
            auto_install=auto_install,
            geosite_path=geosite_path,
        )
        self.v2ray_path = None
        self.is_initialized = True

    def set_config_string(self, config_str):
        """Validate and retain a config using the legacy method name.

        Args:
            config_str (str): Share URI, JSON, or supported config path.

        Returns:
            dict[str, str]: Legacy success object ``{"result": "valid"}``.

        Raises:
            ConfigurationError: If parsing or validation fails.
        """
        self.set_config(config_str)
        return {"result": "valid"}

    def validate_config(self, config_input=None):
        """Validate a configuration using the legacy method name.

        Args:
            config_input (str | None): Explicit config or retained config.

        Returns:
            dict[str, object]: Native validation response.
        """
        return self.validate(config_input)

    def get_status(self):
        """Return the current runtime status.

        Returns:
            str: Native lifecycle state.
        """
        return self.status

    def test_connection(self, config_str, timeout=10, test_url=""):
        """Measure one configuration through the native latency service.

        Args:
            config_str (str): Share URI or raw configuration.
            timeout (int): Timeout in seconds.
            test_url (str): Optional latency endpoint.

        Returns:
            int: Latency in milliseconds.

        Raises:
            NativeCallError: If the individual result is nonnumeric.
        """
        result = self.test_latency([config_str], test_url=test_url, timeout=timeout)[0]
        if not isinstance(result, int):
            from .errors import NativeCallError

            raise NativeCallError(result)
        return result

    def test_configs(self, configs, timeout=10, test_url=""):
        """Measure multiple configurations using the legacy batch method.

        Args:
            configs (str | os.PathLike[str] | Iterable[str]): Config source.
            timeout (int): Timeout in seconds.
            test_url (str): Optional latency endpoint.

        Returns:
            list[int | str]: Ordered latencies and native per-item errors.
        """
        return self.test_latency(configs, test_url=test_url, timeout=timeout)

    def get_total_traffics(self):
        """Return cumulative traffic in the legacy dictionary shape.

        Returns:
            dict[str, int]: ``uplink`` and ``downlink`` byte counters.
        """
        value = self.traffic
        return {"uplink": value.uplink, "downlink": value.downlink}

    def get_realtime_speed(self):
        """Return realtime speed in the legacy dictionary shape.

        Returns:
            dict[str, float]: Upload and download bytes per second.
        """
        value = self.speed
        return {"uplinkSpeed": value.uplink, "downlinkSpeed": value.downlink}

    def get_version_info(self):
        """Return version metadata with native JSON field names.

        Returns:
            dict[str, object]: Code version, upstream version, and release date.
        """
        value = self.version_info
        return {
            "codeVersion": value.code_version,
            "version": value.version,
            "releaseDate": value.release_date,
        }

    def json_to_config_string(self, config):
        """Convert JSON to a share URI through the legacy method name.

        Args:
            config (str | Mapping[str, object]): JSON text or serializable mapping.

        Returns:
            str: Supported share URI.
        """
        return self.to_share_uri(config)

    def set_log_level(self, level):
        """Configure the native log level.

        Args:
            level (str): Native level such as ``warning`` or ``debug``.

        Returns:
            None: Process-global native logging is updated.
        """
        self.configure_logging(level=level)

    def set_log_output(self, path):
        """Configure the native log destination.

        Args:
            path (str | os.PathLike[str]): Writable log file path.

        Returns:
            None: Process-global native logging is updated.
        """
        self.configure_logging(output=path)

    def reset_network_proxy(self):
        """Stop the runtime without changing operating-system proxy settings.

        Returns:
            str: Runtime status after shutdown.

        Notes:
            V2Root 2.0 intentionally does not mutate system proxy configuration.
        """
        return self.stop()
