V2Root Python SDK 2.0
=====================

V2Root is a Python 3.10+ SDK for embedding, managing, and operating
`V2Root Core <https://github.com/v2RayRoot/V2Root-Core>`_.

Version 2.0:

* downloads the official core automatically;
* verifies release package SHA-256 manifests;
* loads the core DLL/SO directly through a memory-safe ``ctypes`` boundary;
* exposes typed runtime, traffic, speed, latency, and version models;
* provides modular subscription parsing, persistence, scheduling, and ranking;
* ships no C source, DLL, SO, or external V2Ray executable.

.. toctree::
   :maxdepth: 2
   :caption: User Guide

   home
   installation
   quick_start
   usage
   subscription
   logger
   supported_options
   troubleshooting

.. toctree::
   :maxdepth: 2
   :caption: Developer Reference

   project_structure
   api
   c_api
