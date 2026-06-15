"""Create, persist, update, filter, and rank subscription configurations.

Inputs:
    Replace ``SUBSCRIPTION_URL`` with an HTTP(S) subscription endpoint.

Output:
    Prints source identity, protocol counts, and the best tested configuration.
"""

from v2root import SubscriptionManager


SUBSCRIPTION_URL = "https://example.com/subscription"


def main() -> None:
    """Manage one subscription using the default persistent repository.

    Returns:
        None: Subscription statistics are printed to standard output.
    """
    with SubscriptionManager() as manager:
        subscription = manager.add_subscription(
            SUBSCRIPTION_URL,
            name="Primary",
            tags=["production"],
            fetch_now=True,
        )
        print("ID:", subscription.id)
        print("Statistics:", subscription.get_statistics())
        print("VLESS:", manager.filter_configs(protocols=["vless"]))
        print("Best tested:", manager.best_config(max_latency=500))


if __name__ == "__main__":
    main()
