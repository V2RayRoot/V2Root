"""Parse, validate, retain, and convert proxy configurations.

Inputs:
    Replace ``SHARE_URI`` with a valid VLESS, VMess, Trojan, or SS URI.

Output:
    Prints validation output and a round-tripped share URI.
"""

from v2root import V2RootClient


SHARE_URI = "vless://REPLACE_ME"


def main() -> None:
    """Demonstrate configuration operations without starting the runtime.

    Returns:
        None: Results are printed to standard output.
    """
    client = V2RootClient()
    normalized_json = client.parse(SHARE_URI)
    print("Validation:", client.validate(normalized_json))

    client.set_config(SHARE_URI)
    print("Share URI:", client.to_share_uri(normalized_json))


if __name__ == "__main__":
    main()
