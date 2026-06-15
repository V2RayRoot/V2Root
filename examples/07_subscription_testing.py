"""Measure subscription configs and persist their latency metadata.

Inputs:
    Replace ``SUBSCRIPTION_URL`` with a real subscription endpoint.

Output:
    Prints each structured latency result and the selected best config.
"""

from v2root import SubscriptionManager, V2RootClient


SUBSCRIPTION_URL = "https://example.com/subscription"


def main() -> None:
    """Fetch a subscription, test every config, and persist measurements.

    Returns:
        None: Measurements and ranking are printed to standard output.
    """
    with SubscriptionManager() as subscriptions:
        source = subscriptions.add_subscription(SUBSCRIPTION_URL)
        client = V2RootClient()
        results = client.test_subscription(subscriptions, source.id, timeout=10)
        for result in results:
            print(result)
        print("Best:", subscriptions.best_config())


if __name__ == "__main__":
    main()
