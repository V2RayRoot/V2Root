"""V2Root Core installation and release management."""

from .manager import CoreManager
from .platforms import current_target
from .transport import HttpTransport

__all__ = ["CoreManager", "HttpTransport", "current_target"]
