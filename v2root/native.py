"""Low-level, memory-safe ctypes binding for the V2Root Core ABI."""

import ctypes
import json
import os
import threading
from pathlib import Path

from .errors import CoreCompatibilityError, NativeCallError


class NativeCore:
    """Own the loaded shared library and enforce the documented V2Root ABI."""
    _ABI = {
        "FreeCString": ([ctypes.c_void_p], None),
        "GetStatus": ([], ctypes.c_void_p),
        "SetLogOutput": ([ctypes.c_char_p], None),
        "SetLogLevel": ([ctypes.c_char_p], None),
        "Start": ([ctypes.c_char_p, ctypes.c_char_p], ctypes.c_void_p),
        "Stop": ([], ctypes.c_void_p),
        "ValidateConfig": ([ctypes.c_char_p, ctypes.c_char_p], ctypes.c_void_p),
        "TestLatency": (
            [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int],
            ctypes.c_void_p,
        ),
        "GetTotalTraffics": ([], ctypes.c_void_p),
        "GetRealtimeSpeed": ([], ctypes.c_void_p),
        "UpdateGeoAssets": ([ctypes.c_char_p], ctypes.c_void_p),
        "GetVersionInfo": ([], ctypes.c_void_p),
        "Parse": ([ctypes.c_char_p], ctypes.c_void_p),
        "ParseVless": ([ctypes.c_char_p], ctypes.c_void_p),
        "ParseTrojan": ([ctypes.c_char_p], ctypes.c_void_p),
        "ParseVmess": ([ctypes.c_char_p], ctypes.c_void_p),
        "ParseShadowsocks": ([ctypes.c_char_p], ctypes.c_void_p),
        "JSONToConfigString": ([ctypes.c_char_p], ctypes.c_void_p),
    }

    def __init__(self, library_path, loader=ctypes.CDLL):
        """Load a shared library and bind the complete V2Root Core ABI.

        Args:
            library_path (str | os.PathLike[str]): Absolute or relative DLL/SO path.
            loader (Callable[[str], object]): ctypes-compatible library loader.

        Raises:
            CoreCompatibilityError: If loading fails or a required symbol is absent.

        Notes:
            On Windows the DLL directory handle is retained for the lifetime of
            this object so dependent libraries remain discoverable.
        """
        self.library_path = Path(library_path).expanduser().resolve()
        self._lock = threading.RLock()
        self._dll_directory = None
        if os.name == "nt" and hasattr(os, "add_dll_directory"):
            self._dll_directory = os.add_dll_directory(str(self.library_path.parent))
        try:
            self._library = loader(str(self.library_path))
        except OSError as exc:
            raise CoreCompatibilityError(
                f"Failed to load V2Root Core {self.library_path}: {exc}"
            ) from exc
        self._bind()

    def _bind(self):
        """Declare ctypes signatures for every required exported function.

        Returns:
            None: Function objects are configured in place.

        Raises:
            CoreCompatibilityError: If any V2Root 2.0 ABI symbol is missing.
        """
        for name, (arguments, result) in self._ABI.items():
            try:
                function = getattr(self._library, name)
            except AttributeError as exc:
                raise CoreCompatibilityError(
                    f"Incompatible V2Root Core: missing ABI symbol {name}"
                ) from exc
            function.argtypes = arguments
            function.restype = result

    @staticmethod
    def encode(value):
        """Encode text for the native C-compatible ABI.

        Args:
            value (str): Python text to encode.

        Returns:
            bytes: UTF-8 bytes automatically null-terminated by ctypes.
        """
        return value.encode("utf-8")

    def take_string(self, pointer):
        """Copy and release one string allocated by V2Root Core.

        Args:
            pointer (int | ctypes.c_void_p | None): Native allocation address.

        Returns:
            str | None: Decoded text, or ``None`` for a null pointer.

        Raises:
            UnicodeDecodeError: If the core returns invalid UTF-8.

        Notes:
            Every non-null pointer is released with ``FreeCString`` in a
            ``finally`` block, including when decoding fails.
        """
        if not pointer:
            return None
        try:
            return ctypes.string_at(pointer).decode("utf-8")
        finally:
            self._library.FreeCString(pointer)

    def string_call(self, name, *arguments):
        """Invoke a string-returning ABI function under a reentrant lock.

        Args:
            name (str): Exported function name.
            *arguments: Already-marshalled ctypes arguments.

        Returns:
            str | None: Copied native result after ownership is released.

        Raises:
            AttributeError: If ``name`` is not a bound export.
            UnicodeDecodeError: If returned text is invalid UTF-8.
        """
        with self._lock:
            return self.take_string(getattr(self._library, name)(*arguments))

    def json_call(self, name, *arguments):
        """Invoke a native operation and decode its JSON result.

        Args:
            name (str): Exported function name.
            *arguments: Already-marshalled ctypes arguments.

        Returns:
            object: JSON-decoded list, dictionary, scalar, or null value.

        Raises:
            NativeCallError: If the result is null, malformed JSON, or contains
                a top-level nonempty ``error`` field.
        """
        raw = self.string_call(name, *arguments)
        if raw is None:
            raise NativeCallError(f"{name} returned an unexpected null pointer")
        try:
            value = json.loads(raw)
        except json.JSONDecodeError as exc:
            raise NativeCallError(f"{name} returned invalid JSON: {raw}") from exc
        if isinstance(value, dict) and value.get("error"):
            raise NativeCallError(str(value["error"]))
        return value

    def void_call(self, name, *arguments):
        """Invoke a void-returning export while serializing native access.

        Args:
            name (str): Exported function name.
            *arguments: Already-marshalled ctypes arguments.

        Returns:
            None: The native function has no result.

        Raises:
            AttributeError: If ``name`` is not a bound export.
        """
        with self._lock:
            getattr(self._library, name)(*arguments)
