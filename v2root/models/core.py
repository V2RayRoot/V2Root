"""Models describing release artifacts and locally installed cores."""

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True, slots=True)
class PlatformTarget:
    """Describe one operating-system and CPU target published by V2Root Core."""
    os: str
    architecture: str
    extension: str

    @property
    def binary_name(self) -> str:
        """Return the canonical shared-library filename.

        Returns:
            str: Filename such as ``xray-linux-amd64.so``.
        """
        return f"xray-{self.os}-{self.architecture}.{self.extension}"

    @property
    def header_name(self) -> str:
        """Return the generated header filename paired with the binary.

        Returns:
            str: Filename such as ``xray-linux-amd64.h``.
        """
        return f"xray-{self.os}-{self.architecture}.h"

    def archive_name(self, version: str) -> str:
        """Build a release archive filename.

        Args:
            version (str): Version suffix used by release packaging.

        Returns:
            str: Expected target-specific ZIP filename.
        """
        return f"V2Root-Core-{self.os}-{self.architecture}-{version}.zip"


@dataclass(frozen=True, slots=True)
class ReleaseAsset:
    """Describe a downloadable asset attached to a GitHub release."""
    name: str
    download_url: str
    size: int


@dataclass(frozen=True, slots=True)
class CoreRelease:
    """Represent normalized V2Root Core release metadata and its named assets."""
    tag: str
    assets: dict[str, ReleaseAsset]
    published_at: str | None = None


@dataclass(frozen=True, slots=True)
class InstalledCore:
    """Describe a verified core installation stored in the local cache."""
    release_tag: str
    directory: Path
    library_path: Path
    header_path: Path
    installed_at: str
    archive_sha256: str


@dataclass(frozen=True, slots=True)
class UpdateResult:
    """Report whether a core update changed the active installed release."""
    previous: InstalledCore | None
    current: InstalledCore
    changed: bool
