"""Use the compatibility facade while migrating an older application.

Inputs:
    Replace ``CONFIG`` with a valid share URI.

Output:
    Prints legacy-shaped status and version dictionaries.
"""

from v2root import V2ROOT


CONFIG = "vless://REPLACE_ME"


def main() -> None:
    """Exercise compatibility names backed by the V2Root 2.0 implementation.

    Returns:
        None: Legacy-shaped values are printed to standard output.
    """
    client = V2ROOT()
    client.set_config_string(CONFIG)
    client.start()
    try:
        print(client.get_status())
        print(client.get_version_info())
    finally:
        client.stop()


if __name__ == "__main__":
    main()
