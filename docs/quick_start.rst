.. _quick_start:

Quick Start Guide
=================

This guide shows you how to quickly set up and use V2ROOT to manage a V2Ray proxy configuration.

Prerequisites
-------------

- Python 3.8 or higher
- V2Ray executable installed (see :ref:`installation`)
- A valid VLESS, VMess, or Shadowsocks configuration string

Installation
------------

Install V2ROOT using pip:

.. code-block:: bash

   pip install v2root

Example: Starting a VLESS Proxy
-------------------------------

Here's a simple example to start a VLESS proxy using V2ROOT:

.. code-block:: python

   from v2root import V2ROOT

   # Initialize V2ROOT
   v2root = V2ROOT()

   # Example VLESS configuration
   vless_config = "vless://uuid@example.com:443?type=tcp&security=tls&sni=example.com"

   # Start the proxy
   v2root.start_proxy(vless_config, http_port=10808, socks_port=1080)

   print("Proxy started! Connect via HTTP (127.0.0.1:10808) or SOCKS (127.0.0.1:1080)")

   # Stop the proxy
   v2root.stop_proxy()

Next Steps
----------

- Learn how to install V2ROOT properly: :ref:`installation`
- Explore more configuration options: :ref:`supported_options`
- Check the Python API: :ref:`api`