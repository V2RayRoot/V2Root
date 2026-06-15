"""Configure Python SDK logging and native V2Root Core logging separately.

Output:
    Creates rotating SDK logs and configures a native core log destination.
"""

import logging
from pathlib import Path

from v2root import V2RootClient, configure_logger, info


def main() -> None:
    """Enable rotating logs without logging credentials or complete configs.

    Returns:
        None: Log records are written to configured destinations.
    """
    log_directory = Path("runtime-logs")
    configure_logger(
        log_level=logging.DEBUG,
        log_to_file=True,
        log_to_console=True,
        log_dir=log_directory,
    )
    info("SDK logging is configured")

    client = V2RootClient()
    client.configure_logging(
        level="warning",
        output=log_directory / "v2root-core.log",
    )


if __name__ == "__main__":
    main()
