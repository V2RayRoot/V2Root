"""Test subscription service coordination across transport and persistence."""

from v2root.subscriptions import SubscriptionManager


class Transport:
    """Provide a deterministic transport that returns one valid VLESS URI."""

    def fetch(self, _url, _timeout):
        """Return static subscription content without performing network I/O.

        Args:
            _url (str): Ignored subscription URL.
            _timeout (int | float): Ignored timeout value.

        Returns:
            str: One valid VLESS configuration line.
        """
        return "vless://id@example.com:443#Node"


def test_update_filter_best_and_persistence(tmp_path):
    """Verify update, latency recording, ranking, filtering, and reload behavior.

    Args:
        tmp_path (pathlib.Path): Isolated persistent storage directory.

    Returns:
        None: Assertions validate coordinated manager behavior.
    """
    manager = SubscriptionManager(storage_dir=tmp_path, transport=Transport())
    subscription = manager.add_subscription("https://example.com/sub")
    config = subscription.configs[0]
    assert manager.record_latency(config.config_string, 90)
    assert manager.best_config().last_latency == 90
    assert manager.filter_configs(protocols=["vless"]) == [config]
    reloaded = SubscriptionManager(storage_dir=tmp_path, transport=Transport())
    assert reloaded.get_subscription(subscription.id).configs[0].last_latency == 90


def test_get_all_configs_returns_strings_and_metadata_is_separate(tmp_path):
    """Verify string and metadata query APIs retain distinct return contracts.

    Args:
        tmp_path (pathlib.Path): Isolated persistent storage directory.

    Returns:
        None: Assertions validate the type of each query result.
    """
    manager = SubscriptionManager(storage_dir=tmp_path, transport=Transport())
    manager.add_subscription("https://example.com/sub")
    assert isinstance(manager.get_all_configs()[0], str)
    assert manager.get_config_metadata()[0].protocol == "vless"
