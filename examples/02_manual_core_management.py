"""Manage V2Root Core releases independently from the runtime client.

Output:
    Prints the active installation, installed versions, and update status.

Raises:
    CoreError: If release discovery, download, verification, or activation fails.
"""

from v2root import CoreManager


def main() -> None:
    """Install the latest core and display available management operations.

    Returns:
        None: Core state is printed to standard output.
    """
    manager = CoreManager()
    active = manager.ensure()
    print("Active:", active.release_tag, active.library_path)
    print("Installed:", [item.release_tag for item in manager.installed()])

    result = manager.update()
    print("Updated:", result.changed, result.current.release_tag)

    # Roll back only when more than one release is installed.
    if len(manager.installed()) > 1:
        previous = manager.rollback()
        print("Rolled back to:", previous.release_tag)


if __name__ == "__main__":
    main()
