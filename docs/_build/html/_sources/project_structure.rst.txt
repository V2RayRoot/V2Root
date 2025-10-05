Project Structure
=================

This section describes the purpose of each file in the V2ROOT project, providing an overview of the codebase.

File Descriptions
-----------------

- **v2root.py**:
  The main Python interface for the V2ROOT library. This file defines the ``V2ROOT`` class, which provides methods to initialize, start, stop, and test V2Ray proxy configurations. It uses ``ctypes`` to interact with the underlying C shared libraries (``libv2root.dll`` on Windows, ``libv2root.so`` on Linux).

- **__init__.py**:
  The package initialization file for the ``v2root`` Python module. This file makes the directory a Python package and exposes the ``V2ROOT`` class for import (e.g., ``from v2root import V2ROOT``).

- **libv2root_common.h**:
  A header file containing common definitions, macros, and utility functions used across the C codebase. This includes error codes, logging macros, and data structures shared between different modules.

- **libv2root_core.c**:
  Implements the core functionality of V2ROOT, such as initializing the V2Ray core, managing the V2Ray process, and handling proxy operations. This file contains the main entry points for the shared library (e.g., ``init_v2ray``, ``start_v2ray``).

- **libv2root_core.h**:
  The header file for ``libv2root_core.c``, defining the function prototypes and data structures for core operations. This includes the API exposed to the Python layer via ``ctypes``.

- **libv2root_linux.c**:
  Contains Linux-specific implementations for managing V2Ray operations, such as process management (fork/exec), file operations, and proxy settings. This file handles platform-specific logic for Linux.

- **libv2root_linux.h**:
  The header file for ``libv2root_linux.c``, defining Linux-specific function prototypes and data structures.

- **libv2root_manage.c**:
  Implements management functions for V2Ray configurations, such as parsing configuration strings (e.g., VLESS, VMess) and managing proxy ports. This file handles the logic for setting up and validating configurations.

- **libv2root_manage.h**:
  The header file for ``libv2root_manage.c``, defining function prototypes for configuration management.

- **libv2root_service.c**:
  Manages the V2Ray service lifecycle, including starting and stopping the V2Ray process. This file handles process monitoring and logging for the V2Ray service.

- **libv2root_service.h**:
  The header file for ``libv2root_service.c``, defining function prototypes for service management.

- **libv2root_shadowsocks.c**:
  Provides support for Shadowsocks configurations within V2ROOT. This file implements the logic to parse and handle Shadowsocks proxy settings, integrating them with V2Ray.

- **libv2root_shadowsocks.h**:
  The header file for ``libv2root_shadowsocks.c``, defining function prototypes for Shadowsocks support.

- **libv2root_utils.c**:
  Contains utility functions used across the project, such as string manipulation, file I/O, and logging. This file provides helper functions to simplify common tasks in other modules.

- **libv2root_utils.h**:
  The header file for ``libv2root_utils.c``, defining utility function prototypes.

- **libv2root_vless.c**:
  Implements support for VLESS protocol configurations. This file handles parsing and managing VLESS configuration strings, enabling V2ROOT to connect to VLESS servers.

- **libv2root_vless.h**:
  The header file for ``libv2root_vless.c``, defining function prototypes for VLESS support.

- **libv2root_vmess.c**:
  Implements support for VMess protocol configurations. This file handles parsing and managing VMess configuration strings, enabling V2ROOT to connect to VMess servers.

- **libv2root_vmess.h**:
  The header file for ``libv2root_vmess.c``, defining function prototypes for VMess support.

- **libv2root_win.c**:
  Contains Windows-specific implementations for managing V2Ray operations, such as process management, file operations, and system proxy settings. This file handles platform-specific logic for Windows.

- **libv2root_win.h**:
  The header file for ``libv2root_win.c``, defining Windows-specific function prototypes and data structures.