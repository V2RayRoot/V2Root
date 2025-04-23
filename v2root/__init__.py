# v2root/__init__.py
"""
V2Root - A Python package to manage v2ray with native extensions.

This package provides a Python interface to manage v2ray proxy software, enabling users to:
- Load and apply V2Ray configurations
- Start and stop V2Ray processes
- Test connection latency
- Ping server (DNS)
- Parse VLESS, VMess, and Shadowsocks configuration strings into V2Ray JSON config files
- Manage system proxy settings for HTTP and SOCKS protocols
- Support for advanced V2Ray features like TCP, HTTP/2, WebSocket, mKCP, QUIC, gRPC, TLS, and Reality

Developed in Python and C for cross-platform compatibility, tested on Windows 10/11 and Linux.

Authors: Project V2Root, Sepehr0Day
Version: 1.1.0
Created: April 2025
License: MIT License
Repository: https://github.com/V2RayRoot/V2Root
Documentation: https://v2root.readthedocs.io
Contact: sphrz2324@gmail.com

Dependencies:
- Python 3.6+
- Windows 10/11 or Linux
- ctypes (standard library)
- urllib.request (standard library)
- colorama (for colored terminal output)
"""

from .v2root import V2ROOT

__all__ = ['V2ROOT']
__version__ = '1.1.0'
__author__ = 'Project V2Root, Sepehr0Day'
__license__ = 'MIT'
__email__ = 'sphrz2324@gmail.com'
__url__ = 'https://github.com/V2RayRoot/V2Root'
__description__ = 'A Python package to manage v2ray with native extensions'