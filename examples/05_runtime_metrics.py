"""Poll typed runtime status, traffic counters, and transfer rates.

Inputs:
    Replace ``CONFIG`` with a working share URI.

Output:
    Prints three metric samples approximately one second apart.
"""

import time

from v2root import V2RootClient


CONFIG = "vless://REPLACE_ME"


def main() -> None:
    """Start a connection and read runtime metrics at a stable interval.

    Returns:
        None: Typed metrics are printed to standard output.
    """
    with V2RootClient() as client:
        client.connect(CONFIG)
        for _ in range(3):
            print("Status:", client.runtime_status)
            print("Traffic:", client.traffic)
            print("Speed:", client.speed)
            time.sleep(1)


if __name__ == "__main__":
    main()
