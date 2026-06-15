"""Measure multiple configurations and preserve per-item failures.

Inputs:
    Replace each item in ``CONFIGS`` with a usable share URI.

Output:
    Prints structured success, latency, and error fields in source order.
"""

from v2root import V2RootClient


CONFIGS = [
    "vless://REPLACE_ME",
    "trojan://REPLACE_ME",
]


def main() -> None:
    """Run a structured latency batch through temporary native proxy instances.

    Returns:
        None: Measurement details are printed to standard output.
    """
    client = V2RootClient()
    for result in client.test_latency_detailed(CONFIGS, timeout=10):
        print(
            result.index,
            result.succeeded,
            result.latency_ms,
            result.error,
        )


if __name__ == "__main__":
    main()
