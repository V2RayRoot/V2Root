"""Operating-system and architecture normalization."""

import platform

from ..errors import CoreCompatibilityError
from ..models import PlatformTarget


_OS = {"Windows": ("windows", "dll"), "Linux": ("linux", "so")}
_ARCH = {
    "AMD64": "amd64",
    "x86_64": "amd64",
    "x86": "386",
    "i386": "386",
    "i686": "386",
    "aarch64": "arm64",
    "arm64": "arm64",
}


def current_target(system=None, machine=None) -> PlatformTarget:
    """Normalize host names into a supported release target.

    Args:
        system (str | None): Optional ``platform.system()`` override.
        machine (str | None): Optional ``platform.machine()`` override.

    Returns:
        PlatformTarget: Normalized OS, architecture, and library extension.

    Raises:
        CoreCompatibilityError: If the platform is unsupported or no release
            artifact is published for the combination.
    """
    system = system or platform.system()
    machine = machine or platform.machine()
    try:
        os_name, extension = _OS[system]
        architecture = _ARCH[machine]
    except KeyError as exc:
        raise CoreCompatibilityError(
            f"Unsupported platform: {system} {machine}"
        ) from exc
    if os_name == "windows" and architecture == "arm64":
        raise CoreCompatibilityError("Windows ARM64 core releases are not available")
    return PlatformTarget(os_name, architecture, extension)
