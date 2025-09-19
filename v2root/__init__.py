# v2root/__init__.py
"""
V2Root - A Python library for managing, using, and testing V2Ray configurations.

This library provides a comprehensive interface for working with V2Ray proxy configurations, enabling users to:
- Parse and validate V2Ray configuration strings (VLESS, VMess, Shadowsocks)
- Test connections and measure latency
- Batch test multiple configurations
- Save and load configurations
- Start and stop V2Ray proxy services
- Manage system proxy settings

Developed in Python with C extensions for high performance, V2Root supports:
- Cross-platform compatibility (Windows and Linux)
- Multiple protocol support (TCP, HTTP/2, WebSocket, mKCP, QUIC, gRPC)
- Security features (TLS, Reality)
- Configuration management

Authors: Project V2Root, Sepehr0Day
Version: 1.1.2
License: MIT License
Repository: https://github.com/V2RayRoot/V2Root
"""

from .v2root import V2ROOT
from .logger import configure_logger, get_logger, debug, info, warning, error, critical

__all__ = ['V2ROOT', 'configure_logger', 'get_logger', 'debug', 'info', 'warning', 'error', 'critical']
__version__ = '1.1.2'
__author__ = 'Project V2Root, Sepehr0Day'
__license__ = 'MIT'
__email__ = 'sphrz2324@gmail.com'
__url__ = 'https://github.com/V2RayRoot/V2Root'
__description__ = 'A Python library for managing, using, and testing V2Ray configurations'