"""Secure, atomic installer and updater for V2Root Core releases."""

import hashlib
import json
import os
import shutil
import tempfile
import threading
import zipfile
from datetime import datetime, timezone
from pathlib import Path

from ..errors import (
    CoreDownloadError,
    CoreIntegrityError,
    CoreNotInstalledError,
)
from ..models import CoreRelease, InstalledCore, ReleaseAsset, UpdateResult
from .platforms import current_target
from .transport import HttpTransport


DEFAULT_REPOSITORY = "v2RayRoot/V2Root-Core"
_MANIFEST = "installation.json"


def _default_cache_dir():
    """Resolve the per-user cache used for managed core releases.

    Returns:
        pathlib.Path: ``V2ROOT_CORE_HOME`` when set, otherwise a platform-aware
        local cache path.
    """
    override = os.environ.get("V2ROOT_CORE_HOME")
    if override:
        return Path(override).expanduser()
    if os.name == "nt":
        base = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData/Local"))
        return base / "V2Root" / "core"
    return Path(os.environ.get("XDG_CACHE_HOME", Path.home() / ".cache")) / "v2root/core"


def _sha256(path):
    """Calculate a file digest using bounded memory.

    Args:
        path (str | os.PathLike[str]): File to hash.

    Returns:
        str: Lowercase hexadecimal SHA-256 digest.

    Raises:
        OSError: If the file cannot be opened or read.
    """
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


class CoreManager:
    """Discover, install, update, activate, and roll back V2Root Core."""

    def __init__(
        self,
        cache_dir=None,
        repository=DEFAULT_REPOSITORY,
        transport=None,
        target=None,
    ):
        """Configure core release discovery and local installation storage.

        Args:
            cache_dir (str | os.PathLike[str] | None): Managed installation root.
            repository (str): GitHub ``owner/repository`` identifier.
            transport (HttpTransport | None): Injectable metadata/download transport.
            target (PlatformTarget | None): Explicit release platform target.

        Raises:
            CoreCompatibilityError: If automatic target detection fails.
        """
        self.cache_dir = Path(cache_dir or _default_cache_dir()).expanduser().resolve()
        self.repository = repository
        self.transport = transport or HttpTransport()
        self.target = target or current_target()
        self._lock = threading.RLock()

    @property
    def api_base(self):
        """Return the configured GitHub REST API base.

        Returns:
            str: Repository releases API prefix.
        """
        return f"https://api.github.com/repos/{self.repository}"

    @property
    def current_file(self):
        """Return the active-release marker path.

        Returns:
            pathlib.Path: ``current`` file inside the managed cache.
        """
        return self.cache_dir / "current"

    def latest_release(self):
        """Fetch metadata for the latest published core release.

        Returns:
            CoreRelease: Normalized release tag, publication date, and assets.

        Raises:
            CoreDownloadError: If metadata cannot be fetched or normalized.
        """
        return self._release_from_payload(
            self.transport.get_json(f"{self.api_base}/releases/latest")
        )

    def get_release(self, tag):
        """Fetch metadata for an explicitly tagged release.

        Args:
            tag (str): Exact GitHub release tag.

        Returns:
            CoreRelease: Normalized release metadata.

        Raises:
            CoreDownloadError: If the tag is unavailable or malformed.
        """
        return self._release_from_payload(
            self.transport.get_json(f"{self.api_base}/releases/tags/{tag}")
        )

    @staticmethod
    def _release_from_payload(payload):
        """Normalize a GitHub release response.

        Args:
            payload (Mapping[str, object]): JSON-decoded GitHub release object.

        Returns:
            CoreRelease: Stable SDK representation.

        Raises:
            CoreDownloadError: If ``tag_name`` is missing.
        """
        tag = payload.get("tag_name")
        if not tag:
            raise CoreDownloadError("GitHub release metadata has no tag_name")
        assets = {}
        for item in payload.get("assets", []):
            name = item.get("name")
            url = item.get("browser_download_url")
            if name and url:
                assets[name] = ReleaseAsset(name, url, int(item.get("size", 0)))
        return CoreRelease(tag, assets, payload.get("published_at"))

    def installed(self):
        """Discover valid cached core installations.

        Returns:
            list[InstalledCore]: Installations ordered by timestamp descending.

        Notes:
            Corrupt unrelated manifests are skipped so one bad historical
            release does not prevent use of healthy installations.
        """
        if not self.cache_dir.is_dir():
            return []
        installations = []
        for manifest in self.cache_dir.glob(f"*/{_MANIFEST}"):
            try:
                installations.append(self._read_manifest(manifest))
            except (OSError, ValueError, KeyError, json.JSONDecodeError):
                continue
        return sorted(installations, key=lambda item: item.installed_at, reverse=True)

    def current(self):
        """Resolve the currently activated core.

        Returns:
            InstalledCore | None: Active valid installation, otherwise ``None``.
        """
        try:
            tag = self.current_file.read_text(encoding="utf-8").strip()
        except FileNotFoundError:
            return None
        manifest = self.cache_dir / tag / _MANIFEST
        if not manifest.is_file():
            return None
        try:
            core = self._read_manifest(manifest)
        except (OSError, ValueError, KeyError, json.JSONDecodeError):
            return None
        return core if core.library_path.is_file() else None

    def ensure(self, auto_install=True):
        """Ensure that a usable active core exists.

        Args:
            auto_install (bool): Install the latest compatible release when absent.

        Returns:
            InstalledCore: Existing or newly installed active core.

        Raises:
            CoreNotInstalledError: If absent and automatic installation is disabled.
            CoreError: If automatic installation fails.
        """
        current = self.current()
        if current:
            return current
        if not auto_install:
            raise CoreNotInstalledError(
                f"V2Root Core is not installed in {self.cache_dir}"
            )
        return self.install()

    def install(self, tag=None, force=False):
        """Download, verify, atomically install, and activate a release.

        Args:
            tag (str | None): Exact release tag, or latest when omitted.
            force (bool): Reinstall even when the release is already cached.

        Returns:
            InstalledCore: Verified active installation.

        Raises:
            CoreDownloadError: If metadata or the target asset is unavailable.
            CoreIntegrityError: If archive layout or checksums are invalid.
            OSError: If local atomic installation operations fail.
        """
        with self._lock:
            release = self.get_release(tag) if tag else self.latest_release()
            existing = self._installed_tag(release.tag)
            if existing and not force:
                self.activate(release.tag)
                return existing
            asset = self._select_asset(release)
            if asset is None:
                available = ", ".join(sorted(release.assets)) or "none"
                raise CoreDownloadError(
                    f"Release {release.tag} has no package for "
                    f"{self.target.os}/{self.target.architecture}; assets: {available}"
                )
            self.cache_dir.mkdir(parents=True, exist_ok=True)
            with tempfile.TemporaryDirectory(
                prefix=".v2root-install-", dir=self.cache_dir
            ) as temporary:
                temporary = Path(temporary)
                archive = self.transport.download(
                    asset.download_url,
                    temporary / asset.name,
                    expected_size=asset.size or None,
                )
                installed = self._extract_and_verify(
                    archive, temporary / "extracted", release.tag
                )
                destination = self.cache_dir / release.tag
                if destination.exists():
                    shutil.rmtree(destination)
                os.replace(installed.directory, destination)
                installed = self._read_manifest(destination / _MANIFEST)
            self.activate(release.tag)
            return installed

    def _select_asset(self, release):
        """Select the unique ZIP matching the manager target.

        Args:
            release (CoreRelease): Release whose assets should be searched.

        Returns:
            ReleaseAsset | None: Unique matching asset, otherwise ``None``.
        """
        prefix = (
            f"V2Root-Core-{self.target.os}-{self.target.architecture}-"
        )
        matches = [
            asset
            for name, asset in release.assets.items()
            if name.startswith(prefix) and name.endswith(".zip")
        ]
        if len(matches) != 1:
            return None
        return matches[0]

    def update(self):
        """Update the active core to the latest release.

        Returns:
            UpdateResult: Previous/current installations and whether they differ.

        Raises:
            CoreError: If release discovery or installation fails.
        """
        previous = self.current()
        latest = self.latest_release()
        if previous and previous.release_tag == latest.tag:
            return UpdateResult(previous, previous, False)
        current = self.install(latest.tag)
        return UpdateResult(previous, current, True)

    def activate(self, tag):
        """Atomically activate an already installed release.

        Args:
            tag (str): Installed release tag.

        Returns:
            InstalledCore: Activated installation.

        Raises:
            CoreNotInstalledError: If the tag is not a valid installation.
            OSError: If the activation marker cannot be replaced.
        """
        core = self._installed_tag(tag)
        if core is None:
            raise CoreNotInstalledError(f"Core release is not installed: {tag}")
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        temp = self.cache_dir / ".current.tmp"
        temp.write_text(tag + "\n", encoding="utf-8")
        os.replace(temp, self.current_file)
        return core

    def rollback(self):
        """Activate the newest valid installation other than the current one.

        Returns:
            InstalledCore: Newly activated previous installation.

        Raises:
            CoreNotInstalledError: If no rollback candidate exists.
        """
        current = self.current()
        candidates = [
            item
            for item in self.installed()
            if current is None or item.release_tag != current.release_tag
        ]
        if not candidates:
            raise CoreNotInstalledError("No previous core release is available")
        return self.activate(candidates[0].release_tag)

    def remove(self, tag):
        """Remove an inactive cached release.

        Args:
            tag (str): Release tag to remove.

        Returns:
            bool: Whether a release directory was deleted.

        Raises:
            CoreIntegrityError: If attempting to remove the active release.
            OSError: If deletion fails.
        """
        with self._lock:
            current = self.current()
            if current and current.release_tag == tag:
                raise CoreIntegrityError("Cannot remove the active core release")
            directory = self.cache_dir / tag
            if directory.is_dir():
                shutil.rmtree(directory)
                return True
            return False

    def _installed_tag(self, tag):
        """Resolve one cached tag when its manifest and binary are valid.

        Args:
            tag (str): Release tag directory name.

        Returns:
            InstalledCore | None: Valid installation or ``None``.
        """
        manifest = self.cache_dir / tag / _MANIFEST
        if not manifest.is_file():
            return None
        try:
            core = self._read_manifest(manifest)
        except (OSError, ValueError, KeyError, json.JSONDecodeError):
            return None
        return core if core.library_path.is_file() else None

    def _extract_and_verify(self, archive, output, tag):
        """Extract and verify a release archive in an isolated directory.

        Args:
            archive (str | os.PathLike[str]): Downloaded release ZIP.
            output (str | os.PathLike[str]): New extraction directory.
            tag (str): Release tag recorded in the installation manifest.

        Returns:
            InstalledCore: Temporary verified installation descriptor.

        Raises:
            CoreIntegrityError: If paths, required files, or checksums are invalid.
            OSError: If extraction or manifest writing fails.
            zipfile.BadZipFile: If the archive is not a valid ZIP.
        """
        output.mkdir()
        archive_sha256 = _sha256(archive)
        with zipfile.ZipFile(archive) as bundle:
            members = [item for item in bundle.infolist() if not item.is_dir()]
            for member in members:
                path = Path(member.filename)
                if path.is_absolute() or ".." in path.parts:
                    raise CoreIntegrityError("Unsafe path found in core archive")
            by_name = {Path(item.filename).name: item for item in members}
            required = {
                self.target.binary_name,
                self.target.header_name,
                "SHA256SUMS",
            }
            missing = required.difference(by_name)
            if missing:
                raise CoreIntegrityError(
                    f"Core archive is missing: {', '.join(sorted(missing))}"
                )
            for name in required:
                destination = output / name
                with bundle.open(by_name[name]) as source, destination.open("wb") as sink:
                    shutil.copyfileobj(source, sink)
        checksums = self._parse_checksums(output / "SHA256SUMS")
        for name in (self.target.binary_name, self.target.header_name):
            expected = checksums.get(name)
            if not expected or _sha256(output / name) != expected:
                raise CoreIntegrityError(f"SHA-256 verification failed for {name}")
        manifest = {
            "release_tag": tag,
            "library": self.target.binary_name,
            "header": self.target.header_name,
            "installed_at": datetime.now(timezone.utc).isoformat(),
            "archive_sha256": archive_sha256,
        }
        (output / _MANIFEST).write_text(
            json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8"
        )
        return self._read_manifest(output / _MANIFEST)

    @staticmethod
    def _parse_checksums(path):
        """Parse a GNU-style SHA256SUMS file.

        Args:
            path (str | os.PathLike[str]): ASCII checksum manifest path.

        Returns:
            dict[str, str]: Basename-to-lowercase-digest mapping.

        Raises:
            OSError: If the manifest cannot be read.
            UnicodeDecodeError: If it is not ASCII text.
        """
        checksums = {}
        for line in path.read_text(encoding="ascii").splitlines():
            parts = line.strip().split()
            if len(parts) == 2 and len(parts[0]) == 64:
                checksums[Path(parts[1].lstrip("*")).name] = parts[0].lower()
        return checksums

    @staticmethod
    def _read_manifest(path):
        """Deserialize a managed installation manifest.

        Args:
            path (str | os.PathLike[str]): ``installation.json`` path.

        Returns:
            InstalledCore: Paths resolved relative to the manifest directory.

        Raises:
            OSError: If the file cannot be read.
            json.JSONDecodeError: If JSON is malformed.
            KeyError: If a required manifest field is absent.
        """
        path = Path(path)
        value = json.loads(path.read_text(encoding="utf-8"))
        directory = path.parent.resolve()
        return InstalledCore(
            release_tag=value["release_tag"],
            directory=directory,
            library_path=directory / value["library"],
            header_path=directory / value["header"],
            installed_at=value["installed_at"],
            archive_sha256=value["archive_sha256"],
        )
