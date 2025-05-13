Usage
=====

This section explains how to use the V2ROOT library to manage V2Ray proxy operations, with examples for common tasks.

Getting Started
---------------

First, import the ``V2ROOT`` class and create an instance. You can specify custom HTTP and SOCKS ports if needed:

.. code-block:: python

   from v2root import V2ROOT

   # Initialize with default ports (HTTP: 2300, SOCKS: 2301)
   client = V2ROOT()

   # Or specify custom ports
   client = V2ROOT(http_port=8080, socks_port=1080)

Starting the V2Ray Proxy
------------------------

To start the V2Ray proxy service, use the ``start`` method. On Linux, this will also display instructions for setting proxy environment variables.

.. code-block:: python

   pid = client.start()
   print(f"V2Ray started with PID: {pid}")

On Linux, you’ll need to manually set the proxy environment variables as shown in the terminal output:

.. code-block:: bash

   export http_proxy=http://127.0.0.1:2300
   export https_proxy=http://127.0.0.1:2300
   export HTTP_PROXY=http://127.0.0.1:2300
   export HTTPS_PROXY=http://127.0.0.1:2300
   export socks_proxy=socks5://127.0.0.1:2301
   export SOCKS_PROXY=socks5://127.0.0.1:2301

Stopping the V2Ray Proxy
------------------------

To stop the V2Ray proxy service, use the ``stop`` method:

.. code-block:: python

   client.stop()
   print("V2Ray stopped successfully!")

On Linux, you’ll need to unset the proxy environment variables:

.. code-block:: bash

   unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY socks_proxy SOCKS_PROXY

Setting a V2Ray Configuration
-----------------------------

You can set a V2Ray configuration string (e.g., VLESS or VMess) using the ``set_config_string`` method:

.. code-block:: python

   config = "vless://user-id@server:443?security=tls&type=tcp#MyVLESS"
   client.set_config_string(config)
   print("Configuration set successfully!")

Testing a Configuration
-----------------------

To test the connectivity and latency of a V2Ray configuration, use the ``test_connection`` method:

.. code-block:: python

   config = "vless://user-id@server:443?security=tls&type=tcp#MyVLESS"
   latency = client.test_connection(config)
   print(f"Latency: {latency}ms")

Pinging a Server
----------------

To measure the latency to a specific server, use the ``ping_server`` method:

.. code-block:: python

   latency = client.ping_server("example.com", 443)
   print(f"Ping latency: {latency}ms")

Testing Multiple Configurations
-------------------------------

You can test multiple V2Ray configurations from a file or list and save the valid ones to a file using the ``test_configs`` method:

.. code-block:: python

   # Test configurations from a file
   best_config = client.test_configs("configs.txt", output_file="valid_configs.txt", min_latency=50, max_latency=500)
   if best_config:
       print(f"Best configuration: {best_config}")

   # Test configurations from a list
   configs = [
       "vless://user1@server1:443?security=tls&type=tcp#Server1",
       "vless://user2@server2:443?security=tls&type=tcp#Server2",
   ]
   best_config = client.test_configs(configs, output_file="valid_configs.txt", min_latency=50, max_latency=500)
   if best_config:
       print(f"Best configuration: {best_config}")

Resetting Network Proxy
-----------------------

To reset the system network proxy settings, use the ``reset_network_proxy`` method:

.. code-block:: python

   client.reset_network_proxy()
   print("Network proxy settings reset successfully!")