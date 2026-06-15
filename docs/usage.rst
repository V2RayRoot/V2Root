Runtime Usage
=============

Configuration Inputs
--------------------

``set_config`` and ``connect`` accept supported share URIs, complete JSON, or
configuration paths accepted by V2Root Core.

.. code-block:: python

   client.set_config("vless://REPLACE_ME")
   client.start()

Lifecycle
---------

.. code-block:: python

   from v2root import RuntimeStatus

   client.start()
   if client.runtime_status is RuntimeStatus.RUNNING:
       print("Connected")
   client.stop()

Metrics
-------

Traffic values are cumulative bytes. Speed values are bytes per second.

.. code-block:: python

   import time

   for _ in range(5):
       print(client.traffic)
       print(client.speed)
       time.sleep(1)

The first speed call establishes a native baseline and can return zero.

Geo Assets
----------

.. code-block:: python

   result = client.update_geo_assets("./resources")
   print(result)

Run network and filesystem operations away from latency-sensitive UI threads.

JSON Conversion
---------------

.. code-block:: python

   normalized = client.parse("vmess://REPLACE_ME")
   share_uri = client.to_share_uri(normalized)

Conversion is intentionally lossy for Xray fields without a share-URI
equivalent.

Core Management
---------------

.. code-block:: python

   from v2root import CoreManager

   manager = CoreManager()
   active = manager.ensure()
   print(manager.installed())
   manager.update()

Do not update, replace, or unload the core while its runtime is active.

Legacy Facade
-------------

.. code-block:: python

   from v2root import V2ROOT

   client = V2ROOT()
   client.set_config_string("vless://REPLACE_ME")
   client.start()
   client.stop()

New applications should use ``V2RootClient``.
