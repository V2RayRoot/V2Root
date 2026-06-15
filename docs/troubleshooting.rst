Troubleshooting
===============

V2Root 2.0 uses typed exceptions. Convert exceptions into structured guidance:

.. code-block:: python

   from v2root import V2RootError, explain_error

   try:
       ...
   except V2RootError as error:
       print(explain_error(error).format())

Core Not Installed
------------------

Enable ``auto_install=True`` or install a release with ``CoreManager.install``.

Core Download Failed
--------------------

Confirm HTTPS access to GitHub and verify that the release contains an asset
for the detected operating system and architecture.

Integrity Verification Failed
-----------------------------

Delete the affected cached release and retry. Never bypass SHA-256 or ZIP path
validation.

Core Is Incompatible
--------------------

Use a binary matching the Python process architecture and V2Root 2.0 ABI.
The DLL/SO and generated header must come from the same release ZIP.

Configuration Failed
--------------------

Validate the URI or JSON without logging credentials. REALITY configurations
require complete security parameters.

Runtime Operation Failed
------------------------

Inspect ``client.runtime_status`` and ``client.version_info``. Only one runtime
may be active in a process. Stop it before updating the core.

Subscription Failed
-------------------

Verify that the endpoint returns plain URI lines or a Base64 document rather
than an HTML login page. Check storage permissions for persistence failures.

More detailed V2.0 guidance is stored in ``ExplainError/2.0.0`` in the source
repository.
