"""Application service coordinating subscription domain components."""

import os
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from .entities import ConfigMetadata, Subscription
from .parser import SubscriptionParser
from .repository import JsonSubscriptionRepository
from .scheduler import SubscriptionScheduler
from .transport import SubscriptionTransport


def _default_directory():
    """Resolve the default per-user subscription storage directory.

    Returns:
        pathlib.Path: ``%APPDATA%/V2Root/subscriptions`` on Windows or
        ``~/.v2root/subscriptions`` on other supported systems.
    """
    if os.name == "nt":
        return Path(os.environ.get("APPDATA", Path.home())) / "V2Root/subscriptions"
    return Path.home() / ".v2root/subscriptions"


class SubscriptionManager:
    """Coordinate subscription transport, parsing, persistence, and scheduling.

    The manager is the application-service boundary for subscriptions. Its
    dependencies are injectable, allowing deterministic tests and alternative
    transports or repositories without changing domain models.
    """
    def __init__(self, storage_dir=None, *, repository=None, parser=None, transport=None):
        """Load persisted subscriptions and restart opted-in schedulers.

        Args:
            storage_dir (str | os.PathLike[str] | None): Directory used by the
                default JSON repository.
            repository (JsonSubscriptionRepository | None): Custom persistence
                implementation. Overrides ``storage_dir`` when supplied.
            parser (SubscriptionParser | None): Custom document parser.
            transport (SubscriptionTransport | None): Custom fetch transport.

        Raises:
            PersistenceError: If an existing subscription file cannot be read.
        """
        self.repository = repository or JsonSubscriptionRepository(storage_dir or _default_directory())
        self.parser = parser or SubscriptionParser()
        self.transport = transport or SubscriptionTransport()
        self.subscriptions = {item.id: item for item in self.repository.list()}
        self._schedulers: dict[str, SubscriptionScheduler] = {}
        for item in self.subscriptions.values():
            if item.auto_update:
                self.start_auto_update(item.id)

    def add_subscription(
        self,
        url,
        name=None,
        auto_update=False,
        update_interval=86400,
        enabled=True,
        priority=0,
        tags=None,
        fetch_now=True,
    ) -> Subscription:
        """Register, optionally fetch, persist, and schedule a subscription.

        Args:
            url (str): Absolute HTTP(S) subscription URL.
            name (str | None): Optional display name.
            auto_update (bool): Start a periodic scheduler after registration.
            update_interval (int): Scheduler interval in seconds.
            enabled (bool): Include this source in aggregate queries.
            priority (int): Application-defined priority value.
            tags (Iterable[str] | None): Source classification labels.
            fetch_now (bool): Fetch and parse the source before returning.

        Returns:
            Subscription: The registered and possibly populated model.

        Raises:
            ValueError: If the URL is duplicated or model values are invalid.
            FetchError: If immediate fetching fails.
            ParseError: If immediate parsing finds no valid configurations.
            PersistenceError: If state cannot be saved.
        """
        if any(item.url == url for item in self.subscriptions.values()):
            raise ValueError("Subscription URL already exists")
        item = Subscription(url, name, auto_update, update_interval, enabled, priority, tags or [])
        self.subscriptions[item.id] = item
        if fetch_now:
            self.update_subscription(item.id)
        else:
            self.repository.save(item)
        if auto_update:
            self.start_auto_update(item.id)
        return item

    def update_subscription(self, subscription_id, timeout=30) -> list[ConfigMetadata]:
        """Fetch, parse, merge, and persist one subscription.

        Args:
            subscription_id (str): Stable identifier of a registered source.
            timeout (int | float): Network timeout in seconds.

        Returns:
            list[ConfigMetadata]: Newly parsed configuration metadata.

        Raises:
            KeyError: If ``subscription_id`` is unknown.
            FetchError: If the source cannot be downloaded.
            ParseError: If downloaded content is invalid.
            PersistenceError: If success or failure state cannot be persisted.
        """
        item = self.require(subscription_id)
        item.total_updates += 1
        try:
            content = self.transport.fetch(item.url, timeout)
            configs = self.parser.parse(content, item.configs)
        except Exception as exc:
            item.last_fetch_success = False
            item.last_error_message = str(exc)
            item.failed_updates += 1
            self.repository.save(item)
            raise
        item.configs = configs
        item.last_update_time = time.time()
        item.last_fetch_success = True
        item.last_error_message = ""
        item.successful_updates += 1
        self.repository.save(item)
        return configs

    def update_all(self, timeout=30, max_workers=4) -> dict[str, list[ConfigMetadata] | Exception]:
        """Update all enabled subscriptions concurrently.

        Args:
            timeout (int | float): Per-subscription network timeout in seconds.
            max_workers (int): Maximum number of worker threads.

        Returns:
            dict[str, list[ConfigMetadata] | Exception]: Mapping from source ID
                to parsed configs or the exception raised by that source.
        """
        results = {}
        with ThreadPoolExecutor(max_workers=max_workers) as pool:
            futures = {
                pool.submit(self.update_subscription, item.id, timeout): item.id
                for item in self.subscriptions.values()
                if item.enabled
            }
            for future in as_completed(futures):
                try:
                    results[futures[future]] = future.result()
                except Exception as exc:
                    results[futures[future]] = exc
        return results

    def remove_subscription(self, subscription_id) -> bool:
        """Stop scheduling and remove one subscription from memory and storage.

        Args:
            subscription_id (str): Stable source identifier.

        Returns:
            bool: Whether the subscription existed in memory.
        """
        scheduler = self._schedulers.pop(subscription_id, None)
        if scheduler:
            scheduler.stop()
        existed = self.subscriptions.pop(subscription_id, None) is not None
        self.repository.delete(subscription_id)
        return existed

    def get_subscription(self, subscription_id):
        """Look up one subscription without raising when it is absent.

        Args:
            subscription_id (str): Stable source identifier.

        Returns:
            Subscription | None: Registered model or ``None``.
        """
        return self.subscriptions.get(subscription_id)

    def require(self, subscription_id) -> Subscription:
        """Look up one subscription and require that it exists.

        Args:
            subscription_id (str): Stable source identifier.

        Returns:
            Subscription: Registered model.

        Raises:
            KeyError: If no source has the requested identifier.
        """
        item = self.get_subscription(subscription_id)
        if item is None:
            raise KeyError(f"Unknown subscription: {subscription_id}")
        return item

    def get_all_configs(self, enabled_only=True) -> list[str]:
        """Flatten share URIs from all selected subscriptions.

        Args:
            enabled_only (bool): Exclude disabled subscriptions when true.

        Returns:
            list[str]: Configuration strings in subscription and source order.
        """
        return [
            config.config_string
            for subscription in self.subscriptions.values()
            if not enabled_only or subscription.enabled
            for config in subscription.configs
        ]

    def get_config_metadata(self, enabled_only=True) -> list[ConfigMetadata]:
        """Flatten metadata objects from all selected subscriptions.

        Args:
            enabled_only (bool): Exclude disabled subscriptions when true.

        Returns:
            list[ConfigMetadata]: References to managed metadata objects.
        """
        return [
            config
            for subscription in self.subscriptions.values()
            if not enabled_only or subscription.enabled
            for config in subscription.configs
        ]

    def get_configs_from_subscription(self, subscription_id):
        """Return share URIs for one source when it exists.

        Args:
            subscription_id (str): Stable source identifier.

        Returns:
            list[str] | None: Config strings or ``None`` for an unknown source.
        """
        item = self.get_subscription(subscription_id)
        return item.get_configs() if item else None

    def filter_configs(self, subscription_tags=None, **filters) -> list[ConfigMetadata]:
        """Filter enabled subscriptions and their configurations.

        Args:
            subscription_tags (Iterable[str] | None): Require at least one
                matching source tag.
            **filters: Keyword filters accepted by
                :meth:`Subscription.filter_configs`.

        Returns:
            list[ConfigMetadata]: Metadata satisfying source and config filters.

        Raises:
            ValueError: If a delegated configuration filter is invalid.
        """
        items = []
        required = set(subscription_tags or [])
        for subscription in self.subscriptions.values():
            if not subscription.enabled or required and not required.intersection(subscription.tags):
                continue
            items.extend(subscription.filter_configs(**filters))
        return items

    def best_config(self, **filters) -> ConfigMetadata | None:
        """Select the lowest-latency tested configuration after filtering.

        Args:
            **filters: Filters accepted by :meth:`filter_configs`.

        Returns:
            ConfigMetadata | None: Best tested candidate, or ``None``.
        """
        candidates = self.filter_configs(**filters)
        tested = [item for item in candidates if item.last_latency >= 0]
        return min(tested, key=lambda item: item.last_latency, default=None)

    def record_latency(self, config_string, latency, success=True) -> bool:
        """Record and persist a test result for the matching share URI.

        Args:
            config_string (str): Exact configuration string to locate.
            latency (int): Measured latency in milliseconds.
            success (bool): Whether the connection test succeeded.

        Returns:
            bool: ``True`` when a matching config was updated.

        Raises:
            PersistenceError: If the owning subscription cannot be saved.
        """
        for item in self.get_config_metadata(enabled_only=False):
            if item.config_string == config_string:
                item.update_test_result(latency, success)
                for subscription in self.subscriptions.values():
                    if item in subscription.configs:
                        self.repository.save(subscription)
                return True
        return False

    def list_subscriptions(self) -> list[dict]:
        """Serialize all registered subscriptions for presentation or export.

        Returns:
            list[dict[str, object]]: JSON-compatible subscription documents.
        """
        return [item.to_dict() for item in self.subscriptions.values()]

    def start_auto_update(self, subscription_id):
        """Enable and start periodic updates for one subscription.

        Args:
            subscription_id (str): Stable source identifier.

        Returns:
            bool: Whether a new scheduler thread was started.

        Raises:
            KeyError: If the source is unknown.
            PersistenceError: If the updated setting cannot be saved.
        """
        item = self.require(subscription_id)
        item.auto_update = True
        scheduler = self._schedulers.get(subscription_id)
        if scheduler is None:
            scheduler = SubscriptionScheduler(
                lambda: self.update_subscription(subscription_id), item.update_interval
            )
            self._schedulers[subscription_id] = scheduler
        self.repository.save(item)
        return scheduler.start()

    def stop_auto_update(self, subscription_id):
        """Disable periodic updates and join the source scheduler.

        Args:
            subscription_id (str): Stable source identifier.

        Returns:
            bool: Whether no scheduler remains running.
        """
        item = self.require(subscription_id)
        item.auto_update = False
        self.repository.save(item)
        scheduler = self._schedulers.pop(subscription_id, None)
        return scheduler.stop() if scheduler else True

    def close(self):
        """Stop all active schedulers and persist their disabled state.

        Returns:
            None: All known schedulers are synchronously requested to stop.
        """
        for subscription_id in list(self._schedulers):
            self.stop_auto_update(subscription_id)

    def __enter__(self):
        """Return the manager for context-managed scheduler ownership.

        Returns:
            SubscriptionManager: This manager instance.
        """
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Stop all schedulers when leaving a context.

        Args:
            exc_type (type[BaseException] | None): Active exception type.
            exc_value (BaseException | None): Active exception instance.
            traceback (types.TracebackType | None): Active traceback.

        Returns:
            None: Exceptions are never suppressed.
        """
        self.close()
