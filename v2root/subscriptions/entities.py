"""Define subscription entities and configuration metadata value objects."""

import hashlib
import time
from dataclasses import dataclass, field
from urllib.parse import urlparse


@dataclass(slots=True)
class ConfigMetadata:
    """Store parsed identity and historical health data for one share URI.

    Attributes:
        config_string (str): Original share URI. It may contain credentials and
            must not be written to logs without redaction.
        protocol (str): Lowercase protocol scheme such as ``vless`` or ``vmess``.
        name (str): Human-readable node label extracted from the URI.
        address (str): Server hostname or IP address without brackets.
        port (int): Remote server port, or ``0`` when the format omits it.
        last_test_time (float): Unix timestamp of the most recent test.
        last_latency (int): Last successful latency in milliseconds, or ``-1``.
        success_count (int): Number of successful recorded tests.
        failure_count (int): Number of failed recorded tests.
        tags (list[str]): User-defined labels used by subscription filters.
    """
    config_string: str
    protocol: str = "unknown"
    name: str = "Unnamed Config"
    address: str = "unknown"
    port: int = 0
    last_test_time: float = 0
    last_latency: int = -1
    success_count: int = 0
    failure_count: int = 0
    tags: list[str] = field(default_factory=list)

    def update_test_result(self, latency: int, success: bool) -> None:
        """Record the outcome of one connectivity test.

        Args:
            latency (int): Measured latency in milliseconds. The value is stored
                only when ``success`` is true.
            success (bool): Whether the test produced a usable connection.

        Returns:
            None: The object is updated in place.

        Notes:
            A failed test stores ``-1`` as ``last_latency`` while preserving
            cumulative success and failure counters.
        """
        self.last_test_time = time.time()
        self.last_latency = latency if success else -1
        self.success_count += int(success)
        self.failure_count += int(not success)

    def get_success_rate(self) -> float:
        """Calculate the ratio of successful tests to all recorded tests.

        Returns:
            float: A value from ``0.0`` through ``1.0``. Untested configs return
            ``0.0`` rather than raising a division error.
        """
        attempts = self.success_count + self.failure_count
        return self.success_count / attempts if attempts else 0.0

    @property
    def tested(self) -> bool:
        """Report whether at least one test result has been recorded.

        Returns:
            bool: ``True`` when ``last_test_time`` is greater than zero.
        """
        return self.last_test_time > 0

    def to_dict(self) -> dict:
        """Serialize this metadata into JSON-compatible primitive values.

        Returns:
            dict[str, object]: A new dictionary suitable for JSON persistence.
                Mutable collections are copied so callers cannot mutate state
                through the serialized representation.
        """
        return {
            "config_string": self.config_string,
            "protocol": self.protocol,
            "name": self.name,
            "address": self.address,
            "port": self.port,
            "last_test_time": self.last_test_time,
            "last_latency": self.last_latency,
            "success_count": self.success_count,
            "failure_count": self.failure_count,
            "tags": list(self.tags),
        }

    @classmethod
    def from_dict(cls, value: dict) -> "ConfigMetadata":
        """Restore metadata from a persisted dictionary.

        Args:
            value (dict[str, object]): Serialized metadata. Unknown keys are
                ignored for forward compatibility.

        Returns:
            ConfigMetadata: A populated metadata instance.

        Raises:
            TypeError: If required dataclass fields have incompatible values.
        """
        return cls(**{key: item for key, item in value.items() if key in cls.__dataclass_fields__})


@dataclass(frozen=True, slots=True)
class SubscriptionStatistics:
    """Summarize configuration health and update history for one subscription.

    Attributes:
        total_configs (int): Number of currently stored configurations.
        tested_configs (int): Number with a nonnegative latency.
        average_latency (float): Mean successful latency in milliseconds.
        protocols (dict[str, int]): Configuration count by protocol scheme.
        total_updates (int): Total attempted source updates.
        successful_updates (int): Updates that fetched and parsed successfully.
        failed_updates (int): Updates that failed during fetch or parsing.
    """
    total_configs: int
    tested_configs: int
    average_latency: float
    protocols: dict[str, int]
    total_updates: int
    successful_updates: int
    failed_updates: int


@dataclass(slots=True)
class Subscription:
    """Represent one remote subscription and its persistent application state.

    The model validates URL and scheduling invariants at construction time. It
    does not perform network I/O itself; fetching, parsing, and persistence are
    coordinated by :class:`SubscriptionManager`.

    Attributes:
        url (str): Absolute HTTP or HTTPS subscription endpoint.
        name (str | None): Display name; defaults to the endpoint hostname.
        auto_update (bool): Whether a manager should schedule periodic updates.
        update_interval (int): Scheduler interval in seconds.
        enabled (bool): Whether aggregate queries should include this source.
        priority (int): Application-defined source priority.
        tags (list[str]): Labels used to select groups of subscriptions.
        configs (list[ConfigMetadata]): Parsed configurations from the source.
        id (str): Stable SHA-256-derived identifier generated from ``url``.
    """
    url: str
    name: str | None = None
    auto_update: bool = False
    update_interval: int = 86400
    enabled: bool = True
    priority: int = 0
    tags: list[str] = field(default_factory=list)
    configs: list[ConfigMetadata] = field(default_factory=list)
    last_update_time: float = 0
    last_fetch_success: bool = False
    last_error_message: str = ""
    total_updates: int = 0
    successful_updates: int = 0
    failed_updates: int = 0
    id: str = field(init=False)

    def __post_init__(self):
        """Validate constructor values and derive stable identity fields.

        Returns:
            None: Validation and normalization mutate the instance in place.

        Raises:
            ValueError: If ``url`` is not an absolute HTTP(S) URL or
                ``update_interval`` is less than one second.
        """
        parsed = urlparse(self.url)
        if parsed.scheme not in {"http", "https"} or not parsed.netloc:
            raise ValueError("Subscription URL must be an absolute HTTP(S) URL")
        if self.update_interval < 1:
            raise ValueError("update_interval must be positive")
        self.id = hashlib.sha256(self.url.encode("utf-8")).hexdigest()[:32]
        self.name = self.name or parsed.netloc

    def get_configs(self) -> list[str]:
        """Return original configuration strings in subscription order.

        Returns:
            list[str]: A new list containing each stored share URI.
        """
        return [item.config_string for item in self.configs]

    def filter_configs(
        self,
        protocols=None,
        min_success_rate=None,
        max_latency=None,
        tags=None,
        name_contains=None,
    ) -> list[ConfigMetadata]:
        """Select configuration metadata matching all supplied constraints.

        Args:
            protocols (Iterable[str] | None): Allowed protocol schemes.
            min_success_rate (float | None): Inclusive success-rate threshold
                between ``0.0`` and ``1.0``.
            max_latency (int | float | None): Inclusive maximum successful
                latency in milliseconds. Untested configs are excluded.
            tags (Iterable[str] | None): At least one tag must match.
            name_contains (str | None): Case-insensitive regular expression
                applied to the human-readable config name.

        Returns:
            list[ConfigMetadata]: Matching objects in their original order.

        Raises:
            ValueError: If ``min_success_rate`` is outside ``0..1`` or the
                regular expression is invalid.
        """
        import re

        pattern = re.compile(name_contains, re.IGNORECASE) if name_contains else None
        result = list(self.configs)
        if protocols:
            allowed = {item.lower() for item in protocols}
            result = [item for item in result if item.protocol in allowed]
        if min_success_rate is not None:
            if not 0 <= min_success_rate <= 1:
                raise ValueError("min_success_rate must be between 0 and 1")
            result = [item for item in result if item.get_success_rate() >= min_success_rate]
        if max_latency is not None:
            result = [item for item in result if 0 <= item.last_latency <= max_latency]
        if tags:
            expected = set(tags)
            result = [item for item in result if expected.intersection(item.tags)]
        if pattern:
            result = [item for item in result if pattern.search(item.name)]
        return result

    def sort_configs(self, by="latency", reverse=False) -> list[ConfigMetadata]:
        """Return configurations ordered by a supported metadata field.

        Args:
            by (str): One of ``latency``, ``success_rate``, ``name``, or
                ``protocol``.
            reverse (bool): Reverse the selected ordering when true.

        Returns:
            list[ConfigMetadata]: A new sorted list; the model is not mutated.

        Raises:
            ValueError: If ``by`` is not a supported field.
        """
        keys = {
            "latency": lambda item: item.last_latency if item.last_latency >= 0 else float("inf"),
            "success_rate": lambda item: item.get_success_rate(),
            "name": lambda item: item.name.casefold(),
            "protocol": lambda item: item.protocol,
        }
        if by not in keys:
            raise ValueError(f"Unsupported sort field: {by}")
        return sorted(self.configs, key=keys[by], reverse=reverse)

    def get_statistics(self) -> SubscriptionStatistics:
        """Calculate an immutable statistical snapshot of current state.

        Returns:
            SubscriptionStatistics: Counts, protocol distribution, update
                totals, and average latency for successfully tested configs.
        """
        tested = [item for item in self.configs if item.last_latency >= 0]
        protocols: dict[str, int] = {}
        for item in self.configs:
            protocols[item.protocol] = protocols.get(item.protocol, 0) + 1
        return SubscriptionStatistics(
            len(self.configs),
            len(tested),
            sum(item.last_latency for item in tested) / len(tested) if tested else 0.0,
            protocols,
            self.total_updates,
            self.successful_updates,
            self.failed_updates,
        )

    def to_dict(self) -> dict:
        """Serialize the complete subscription into JSON-compatible values.

        Returns:
            dict[str, object]: Persistent source settings, update state, and
                serialized configuration metadata.
        """
        return {
            key: value
            for key, value in {
                "id": self.id,
                "url": self.url,
                "name": self.name,
                "auto_update": self.auto_update,
                "update_interval": self.update_interval,
                "enabled": self.enabled,
                "priority": self.priority,
                "tags": list(self.tags),
                "configs": [item.to_dict() for item in self.configs],
                "last_update_time": self.last_update_time,
                "last_fetch_success": self.last_fetch_success,
                "last_error_message": self.last_error_message,
                "total_updates": self.total_updates,
                "successful_updates": self.successful_updates,
                "failed_updates": self.failed_updates,
            }.items()
        }

    @classmethod
    def from_dict(cls, value: dict) -> "Subscription":
        """Restore a subscription from repository data.

        Args:
            value (dict[str, object]): JSON-decoded subscription document.

        Returns:
            Subscription: A validated source with restored runtime statistics.

        Raises:
            KeyError: If the required ``url`` field is absent.
            ValueError: If persisted URL or interval values are invalid.
            TypeError: If persisted fields have incompatible types.
        """
        subscription = cls(
            url=value["url"],
            name=value.get("name"),
            auto_update=value.get("auto_update", False),
            update_interval=value.get("update_interval", 86400),
            enabled=value.get("enabled", True),
            priority=value.get("priority", 0),
            tags=list(value.get("tags", [])),
        )
        subscription.configs = [ConfigMetadata.from_dict(item) for item in value.get("configs", [])]
        for field_name in (
            "last_update_time",
            "last_fetch_success",
            "last_error_message",
            "total_updates",
            "successful_updates",
            "failed_updates",
        ):
            setattr(subscription, field_name, value.get(field_name, getattr(subscription, field_name)))
        return subscription
