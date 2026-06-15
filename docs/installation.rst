Installation
============

Requirements
------------

* Python 3.10 or newer
* Windows or Linux on a supported architecture
* HTTPS access to GitHub for automatic first-use core installation

Install from PyPI:

.. code-block:: console

   python -m pip install --upgrade v2root

Verify:

.. code-block:: python

   import v2root

   print(v2root.__version__)

Automatic Core Installation
---------------------------

The wheel does not contain a native binary. The first ``V2RootClient()``
construction installs the latest official compatible V2Root Core when needed:

.. code-block:: python

   from v2root import V2RootClient

   client = V2RootClient()
   print(client.library_path)

Default cache locations:

* Windows: ``%LOCALAPPDATA%\\V2Root\\core``
* Linux: ``$XDG_CACHE_HOME/v2root/core`` or ``~/.cache/v2root/core``

Set ``V2ROOT_CORE_HOME`` to override the core cache.

Offline and Pinned Installation
-------------------------------

Install a known release before creating the client:

.. code-block:: python

   from v2root import CoreManager, V2RootClient

   manager = CoreManager()
   manager.install("v2root-v26.3.27")
   client = V2RootClient(core_manager=manager, auto_install=False)

Load a development build directly:

.. code-block:: python

   client = V2RootClient(core_path="/opt/v2root/xray-linux-amd64.so")

The explicit binary must match the operating system, architecture, and V2Root
2.0 ABI.

Application Dependencies
------------------------

V2Root depends only on ``colorama``. Native V2Root Core is managed separately
from Python package dependencies.
