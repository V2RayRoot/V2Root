Overview
========

V2Root 2.0 is a managed SDK around the process-global V2Root Core runtime.
Applications interact with Python objects; the SDK handles core discovery,
installation, ABI declarations, native string ownership, and error conversion.

Architecture
------------

``V2RootClient``
  High-level API for configurations, lifecycle, metrics, latency, geo assets,
  and core updates.

``CoreManager``
  Secure release discovery, installation, activation, update, rollback, and
  removal.

``NativeCore``
  Internal ABI boundary. Every native string is copied and released using
  ``FreeCString``.

``SubscriptionManager``
  Subscription fetching, parsing, atomic persistence, scheduling, filtering,
  latency history, and best-node selection.

Core Lifecycle
--------------

The embedded runtime is process-global and supports one active engine:

.. code-block:: text

   STOPPED -> STARTING -> RUNNING -> STOPPING -> STOPPED

Serialize lifecycle operations and stop the runtime before updating or
unloading its shared library.

Security Model
--------------

Core installation uses official GitHub Releases metadata, exact platform asset
selection, bounded downloads, ZIP path validation, required-file validation,
SHA-256 verification, temporary extraction, and atomic activation.

Share URIs may contain reusable credentials. Do not log complete URIs,
configuration JSON, UUIDs, passwords, private keys, or security query
parameters.

Compatibility
-------------

``V2ROOT`` remains available as a migration facade. It delegates to the 2.0
implementation and does not restore the old C wrapper or external executable.

Version
-------

SDK version: ``2.0.0``

Minimum Python: ``3.10``
