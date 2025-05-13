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
   v2 = V2ROOT()

   # Set a VLESS string
   vless_str = "vless://your-uuid@your-server:443?security=tls&type=tcp"
   v2.set_config_string(vless_str)

   # Start V2Ray
   v2.start()

   # Stop V2Ray when done
   v2.stop()

Next Steps
----------

- Learn how to install V2ROOT properly: :ref:`installation`
- Explore more configuration options: :ref:`supported_options`
- Check the Python API: :ref:`api`