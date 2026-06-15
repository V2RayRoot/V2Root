"""Test secure installation and lifecycle management of cached core releases."""

import pytest

from v2root.core import CoreManager
from v2root.errors import CoreIntegrityError, CoreNotInstalledError
from v2root.models import PlatformTarget

from conftest import FakeCoreTransport, create_archive


def test_install_update_and_rollback(tmp_path):
    """Verify install, update, activation, and rollback across two fake releases.

    Args:
        tmp_path (pathlib.Path): Isolated pytest filesystem directory.

    Returns:
        None: Assertions validate binary contents and active release tags.
    """
    target = PlatformTarget("windows", "amd64", "dll")
    archives = {}
    for tag in ("v1", "v2"):
        archives[tag] = tmp_path / f"{tag}.zip"
        create_archive(archives[tag], target)
    manager = CoreManager(
        cache_dir=tmp_path / "cache",
        transport=FakeCoreTransport(archives, ["v1", "v2"]),
        target=target,
    )
    assert manager.install("v1").library_path.read_bytes() == b"binary"
    assert manager.update().current.release_tag == "v2"
    assert manager.rollback().release_tag == "v1"


@pytest.mark.parametrize("options", [{"corrupt": True}, {"unsafe": True}])
def test_rejects_invalid_archives(tmp_path, options):
    """Verify checksum corruption and unsafe ZIP paths are rejected.

    Args:
        tmp_path (pathlib.Path): Isolated pytest filesystem directory.
        options (dict[str, bool]): Archive corruption mode supplied by pytest.

    Returns:
        None: The expected integrity exception is asserted.
    """
    target = PlatformTarget("windows", "amd64", "dll")
    archive = tmp_path / "bad.zip"
    create_archive(archive, target, **options)
    manager = CoreManager(
        cache_dir=tmp_path / "cache",
        transport=FakeCoreTransport({"bad": archive}, ["bad"]),
        target=target,
    )
    with pytest.raises(CoreIntegrityError):
        manager.install("bad")


def test_auto_install_can_be_disabled(tmp_path):
    """Verify callers can require a preinstalled core without network fallback.

    Args:
        tmp_path (pathlib.Path): Empty isolated core cache.

    Returns:
        None: The expected missing-core exception is asserted.
    """
    manager = CoreManager(cache_dir=tmp_path, target=PlatformTarget("windows", "amd64", "dll"))
    with pytest.raises(CoreNotInstalledError):
        manager.ensure(auto_install=False)
