Native ABI Integration
======================

The Python SDK loads V2Root Core directly through ``ctypes``. Applications that
need another host language should use the generated header included in the
matching V2Root Core release ZIP.

Package Contents
----------------

Each native release ZIP contains:

* one platform-specific DLL or SO;
* the generated header for that exact binary;
* ``SHA256SUMS`` covering the binary and header.

Memory Ownership
----------------

Every non-null textual pointer returned by the core must be released exactly
once with ``FreeCString``. Do not use C ``free``, a language runtime allocator,
or an unrelated Windows allocator.

Lifecycle
---------

The runtime is process-global. Serialize ``Start`` and ``Stop`` and do not
unload the shared library while calls or runtime activity remain.

Python users should not call this layer directly. ``NativeCore`` centralizes
ABI declarations, UTF-8 handling, JSON decoding, locking, and native string
ownership.
