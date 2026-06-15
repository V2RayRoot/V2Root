"""Use application-private core, subscription, and geo-asset directories.

Output:
    Prints resolved storage and core installation paths.
"""

from pathlib import Path

from v2root import CoreManager, SubscriptionManager, V2RootClient


def main() -> None:
    """Construct SDK services with explicit application-owned paths.

    Returns:
        None: Resolved paths are printed to standard output.
    """
    data = Path("application-data").resolve()
    core_manager = CoreManager(cache_dir=data / "core")
    subscriptions = SubscriptionManager(storage_dir=data / "subscriptions")
    client = V2RootClient(
        core_manager=core_manager,
        geosite_path=data / "geo" / "geosite.dat",
    )
    print("Core:", client.library_path)
    print("Subscriptions:", subscriptions.repository.directory)
    subscriptions.close()


if __name__ == "__main__":
    main()
