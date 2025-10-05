Welcome to V2ROOT Documentation!
================================

V2ROOT is a Python library designed to manage V2Ray proxy operations on Windows and Linux platforms. It provides a robust interface to initialize, start, stop, and test V2Ray proxy configurations, supporting both HTTP and SOCKS proxies. The library aims to streamline the process of connecting to V2Ray servers, making it easier for users to manage multiple configurations, test connectivity, and measure latency.

New in Version 1.2.0
--------------------

- **Subscription Management**: Full-featured subscription management with automatic updates, filtering, and metadata tracking
- **Configuration Metadata**: Track latency, success rates, and tags for each configuration
- **Advanced Filtering**: Filter configurations by protocol, latency, success rate, tags, and name patterns
- **Auto-Update**: Automatic subscription updates with configurable intervals
- **Enhanced Logging**: Improved logging system with function call tracking and colored output

About the Project
-----------------

V2ROOT was created to address the complexities of managing V2Ray proxy configurations, offering a unified solution for both Windows and Linux environments. The project integrates low-level system operations with a high-level Python interface, enabling users to automate proxy management tasks efficiently.

Project Goals
-------------

- **Simplify V2Ray Configuration Management**: V2ROOT makes it easy to connect to V2Ray configurations (e.g., VLESS, VMess, Shadowsocks) by providing simple methods to set, test, and manage proxy settings. It eliminates the need for manual configuration file edits and complex command-line interactions.

- **Subscription Management**: Manage multiple V2Ray subscriptions with automatic updates, filtering, and metadata tracking. Easily switch between configurations and find the best server for your needs.

- **Cross-Platform Support**: The library ensures seamless operation on both Windows and Linux, handling platform-specific nuances like proxy settings and process management.

- **Performance Testing**: V2ROOT allows users to test multiple configurations for connectivity and latency, helping them identify the best server for their needs.

- **Extensibility**: By exposing a C API through shared libraries (DLL on Windows, SO on Linux), V2ROOT enables developers to integrate its functionality into other projects, regardless of the programming language.

Development Languages
---------------------

V2ROOT is developed using a combination of **Python** and **C**:

- **Python**: The high-level interface is written in Python, providing an easy-to-use API for end-users to manage V2Ray operations. This includes methods for starting/stopping the proxy, testing configurations, pinging servers, and managing subscriptions.

- **C**: The core functionality is implemented in C for performance and direct system interaction. This includes managing V2Ray processes, handling network operations, and interfacing with system proxy settings. The C code is compiled into shared libraries (``libv2root.dll`` on Windows, ``libv2root.so`` on Linux) that the Python layer interacts with via ``ctypes``.

Project Information
-------------------

**Authors**: Project V2root | Sepehr0Day

**Release Date**: January 2025

**Version**: 1.2.0

**About the Project**: V2ROOT was created to address the complexities of managing V2Ray proxy configurations, offering a unified solution for both Windows and Linux environments. The project integrates low-level system operations with a high-level Python interface, enabling users to automate proxy management tasks efficiently.

**Links**:

- `V2ROOT Repository <https://github.com/V2RayRoot/V2Root/>`_
- `V2ROOT Package <https://pypi.org/project/v2root/>`_
- `Support Channel <https://t.me/DevSepehr>`_
