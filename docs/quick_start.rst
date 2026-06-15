Quick Start
===========

Connect
-------

.. code-block:: python

   from v2root import V2RootClient

   config = "vless://REPLACE_ME"

   with V2RootClient(http_port=2300, socks_port=2301) as client:
       client.connect(config)
       print(client.runtime_status)
       print(client.version_info)

The context manager stops an active runtime during normal and exceptional exit.

Validate Without Starting
-------------------------

.. code-block:: python

   client = V2RootClient()
   normalized = client.parse("trojan://REPLACE_ME")
   print(client.validate(normalized))

Latency
-------

.. code-block:: python

   results = client.test_latency_detailed(
       ["vless://REPLACE_ME", "trojan://REPLACE_ME"],
       timeout=10,
   )

   for result in results:
       if result.succeeded:
           print(result.latency_ms)
       else:
           print(result.error)

Core Update
-----------

.. code-block:: python

   client.stop()
   update = client.update_core()
   print(update.changed, update.current.release_tag)
