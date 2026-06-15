"""Handle typed V2Root 2.0 exceptions and render structured guidance.

Output:
    Prints a stable explanation code and user-facing remediation text.
"""

from v2root import V2RootClient, V2RootError, explain_error


def main() -> None:
    """Demonstrate broad application-boundary error handling.

    Returns:
        None: Any caught SDK error is converted to safe diagnostic output.
    """
    try:
        client = V2RootClient(auto_install=False)
        client.connect("vless://REPLACE_ME")
    except V2RootError as error:
        explanation = explain_error(error)
        print("Code:", explanation.code)
        print("Retryable:", explanation.retryable)
        print(explanation.format())


if __name__ == "__main__":
    main()
