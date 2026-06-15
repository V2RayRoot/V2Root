"""Test atomic JSON persistence for subscription domain models."""

from v2root.subscriptions import JsonSubscriptionRepository, Subscription


def test_atomic_round_trip(tmp_path):
    """Verify saved subscriptions load correctly without temporary-file residue.

    Args:
        tmp_path (pathlib.Path): Isolated repository directory.

    Returns:
        None: Assertions validate identity and atomic cleanup.
    """
    repository = JsonSubscriptionRepository(tmp_path)
    subscription = Subscription("https://example.com/sub")
    repository.save(subscription)
    loaded = repository.list()
    assert loaded[0].id == subscription.id
    assert not list(tmp_path.glob("*.tmp"))
