"""Connect with automatic core installation and guaranteed context cleanup.

Inputs:
    Replace ``CONFIG`` with a supported share URI.

Output:
    Prints the active core version and runtime status.

Raises:
    V2RootError: If core installation, parsing, validation, or startup fails.
"""

from v2root import V2RootClient


CONFIG = "vless://REPLACE_ME"


def main() -> None:
    """Start one proxy session and stop it automatically.

    Returns:
        None: Runtime information is printed to standard output.
    """
    with V2RootClient(http_port=2300, socks_port=2301) as client:
        client.connect(CONFIG)
        print("Version:", client.version_info)
        print("Status:", client.runtime_status.value)


if __name__ == "__main__":
    main()
