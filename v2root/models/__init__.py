"""Public immutable value objects returned by the V2Root SDK."""

from .core import CoreRelease, InstalledCore, PlatformTarget, ReleaseAsset, UpdateResult
from .runtime import RealtimeSpeed, RuntimeStatus, Traffic, VersionInfo
from .testing import LatencyResult

__all__ = [
    "CoreRelease",
    "InstalledCore",
    "LatencyResult",
    "PlatformTarget",
    "RealtimeSpeed",
    "ReleaseAsset",
    "RuntimeStatus",
    "Traffic",
    "UpdateResult",
    "VersionInfo",
]
