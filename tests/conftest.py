"""Provide reusable in-memory native and release fixtures for the test suite."""

import ctypes
import hashlib
import zipfile

import pytest

from v2root.client import V2RootClient
from v2root.native import NativeCore


class FakeFunction:
    """Mimic a mutable ctypes function object with assignable ABI attributes."""

    def __init__(self, callback):
        """Store the Python callback invoked by the fake function.

        Args:
            callback (Callable[..., object]): Implementation called by
                :meth:`__call__`.

        Returns:
            None: ABI metadata fields are initialized to ``None``.
        """
        self.callback = callback
        self.argtypes = None
        self.restype = None

    def __call__(self, *args):
        """Forward positional arguments to the stored implementation.

        Args:
            *args: Positional values supplied by the native wrapper.

        Returns:
            object: Callback result.
        """
        return self.callback(*args)


class FakeLibrary:
    """Implement the complete V2Root Core ABI using Python-managed buffers."""

    def __init__(self):
        """Create fake exported functions and initialize runtime state.

        Returns:
            None: Function attributes are ready for ``NativeCore._bind``.
        """
        self.buffers = {}
        self.status = "STOPPED"
        self.FreeCString = FakeFunction(self._free)
        self.GetStatus = FakeFunction(lambda: self._string(self.status))
        self.SetLogOutput = FakeFunction(lambda _value: None)
        self.SetLogLevel = FakeFunction(lambda _value: None)
        self.Start = FakeFunction(self._start)
        self.Stop = FakeFunction(self._stop)
        self.ValidateConfig = FakeFunction(lambda *_: self._string('{"result":"valid"}'))
        self.TestLatency = FakeFunction(lambda *_: self._string('["42","[ERROR]bad"]'))
        self.GetTotalTraffics = FakeFunction(lambda: self._string('{"uplink":1,"downlink":2}'))
        self.GetRealtimeSpeed = FakeFunction(
            lambda: self._string('{"uplinkSpeed":3.5,"downlinkSpeed":4.5}')
        )
        self.UpdateGeoAssets = FakeFunction(lambda _path: self._string("{}"))
        self.GetVersionInfo = FakeFunction(
            lambda: self._string('{"codeVersion":7,"version":"v-test","releaseDate":"today"}')
        )
        self.Parse = FakeFunction(lambda _options: self._string('{"outbounds":[]}'))
        self.ParseVless = FakeFunction(lambda _: self._string(""))
        self.ParseTrojan = FakeFunction(lambda _: self._string(""))
        self.ParseVmess = FakeFunction(lambda _: self._string(""))
        self.ParseShadowsocks = FakeFunction(lambda _: self._string(""))
        self.JSONToConfigString = FakeFunction(lambda _: self._string("vless://x"))

    def _string(self, value):
        """Allocate a stable ctypes buffer and return its integer address.

        Args:
            value (str): UTF-8 text to expose through the fake ABI.

        Returns:
            int: Address retained until ``FreeCString`` is called.
        """
        buffer = ctypes.create_string_buffer(value.encode())
        address = ctypes.addressof(buffer)
        self.buffers[address] = buffer
        return address

    def _free(self, pointer):
        """Release a fake native allocation from the buffer registry.

        Args:
            pointer (int | ctypes.c_void_p): Address supplied by ``NativeCore``.

        Returns:
            None: The allocation is removed when present.
        """
        address = pointer.value if hasattr(pointer, "value") else pointer
        self.buffers.pop(address, None)

    def _start(self, *_):
        """Transition the fake runtime to ``RUNNING`` and report native success.

        Args:
            *_: Ignored encoded config and options arguments.

        Returns:
            None: A null native pointer represents successful startup.
        """
        self.status = "RUNNING"
        return None

    def _stop(self):
        """Transition the fake runtime to ``STOPPED`` and report native success.

        Returns:
            None: A null native pointer represents successful shutdown.
        """
        self.status = "STOPPED"
        return None


@pytest.fixture
def client(tmp_path):
    """Build a high-level client backed by the in-memory ABI.

    Args:
        tmp_path (pathlib.Path): Isolated path used as the nominal core path.

    Returns:
        tuple[V2RootClient, FakeLibrary]: Client and inspectable fake library.
    """
    library = FakeLibrary()
    native = NativeCore(tmp_path / "core.dll", loader=lambda _: library)
    instance = V2RootClient(
        core_path=tmp_path / "core.dll",
        native_factory=lambda _: native,
    )
    return instance, library


def create_archive(path, target, unsafe=False, corrupt=False):
    """Create a release ZIP with optional integrity or path traversal defects.

    Args:
        path (pathlib.Path): Destination ZIP path.
        target (PlatformTarget): Determines binary and header filenames.
        unsafe (bool): Add a parent-directory ZIP member when true.
        corrupt (bool): Write an invalid binary checksum when true.

    Returns:
        None: The archive is written to ``path``.
    """
    binary, header = b"binary", b"header"
    first = "0" * 64 if corrupt else hashlib.sha256(binary).hexdigest()
    checksums = (
        f"{first}  {target.binary_name}\n"
        f"{hashlib.sha256(header).hexdigest()}  {target.header_name}\n"
    )
    with zipfile.ZipFile(path, "w") as archive:
        archive.writestr("package/" + target.binary_name, binary)
        archive.writestr("package/" + target.header_name, header)
        archive.writestr("package/SHA256SUMS", checksums)
        if unsafe:
            archive.writestr("../outside", b"bad")


class FakeCoreTransport:
    """Serve deterministic release metadata and archives without network access."""

    def __init__(self, archives, tags):
        """Store release archives and their chronological tag order.

        Args:
            archives (dict[str, pathlib.Path]): Archive path by release tag.
            tags (list[str]): Ordered tags; the final item is considered latest.

        Returns:
            None: Inputs are retained for later transport calls.
        """
        self.archives = archives
        self.tags = tags

    def get_json(self, url):
        """Return a GitHub-compatible release payload for the requested URL.

        Args:
            url (str): Latest-release or tag endpoint.

        Returns:
            dict[str, object]: Minimal release metadata consumed by CoreManager.
        """
        tag = url.rsplit("/", 1)[-1]
        tag = self.tags[-1] if tag == "latest" else tag
        name = f"V2Root-Core-windows-amd64-{tag}.zip"
        return {
            "tag_name": tag,
            "assets": [{
                "name": name,
                "browser_download_url": f"https://example/{tag}",
                "size": self.archives[tag].stat().st_size,
            }],
        }

    def download(self, url, destination, **_):
        """Copy the selected fixture archive to the requested destination.

        Args:
            url (str): Synthetic URL whose final segment is a release tag.
            destination (pathlib.Path): Target download path.
            **_: Ignored transport validation options.

        Returns:
            pathlib.Path: ``destination`` after writing archive bytes.
        """
        destination.write_bytes(self.archives[url.rsplit("/", 1)[-1]].read_bytes())
        return destination
