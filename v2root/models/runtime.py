"""Runtime status, version, traffic, and speed models."""

from dataclasses import dataclass
from enum import Enum
from typing import Any


class RuntimeStatus(str, Enum):
    """Enumerate lifecycle states exposed by the process-global native runtime."""
    STOPPED = "STOPPED"
    STARTING = "STARTING"
    RUNNING = "RUNNING"
    STOPPING = "STOPPING"


@dataclass(frozen=True, slots=True)
class VersionInfo:
    """Expose build metadata reported by the loaded V2Root Core binary."""
    code_version: int
    version: str
    release_date: str | None

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "VersionInfo":
        """Create a typed version record from native JSON.

        Args:
            value (dict[str, Any]): Decoded ``GetVersionInfo`` response.

        Returns:
            VersionInfo: Normalized metadata with defensive defaults.
        """
        return cls(
            code_version=int(value.get("codeVersion", 0)),
            version=str(value.get("version", "")),
            release_date=value.get("releaseDate"),
        )


@dataclass(frozen=True, slots=True)
class Traffic:
    """Contain cumulative uploaded and downloaded byte counters."""
    uplink: int
    downlink: int


@dataclass(frozen=True, slots=True)
class RealtimeSpeed:
    """Contain current upload and download rates measured in bytes per second."""
    uplink: float
    downlink: float
