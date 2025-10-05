# v2root/__init__.py
"""
V2ROOT - A Python library for managing, using, and testing V2Ray configurations.

This library provides a comprehensive interface for working with V2Ray proxy configurations, enabling users to:
- Parse and validate V2Ray configuration strings (VLESS, VMess, Shadowsocks)
- Test connections and measure latency
- Batch test multiple configurations
- Save and load configurations
- Start and stop V2Ray proxy services
- Manage system proxy settings
- Handle subscription-based configurations

Developed in Python with C extensions for high performance, V2Root supports:
- Cross-platform compatibility (Windows and Linux)
- Multiple protocol support (TCP, HTTP/2, WebSocket, mKCP, QUIC, gRPC)
- Security features (TLS, Reality)
- Configuration management
- Subscription management

Authors: Project V2Root, Sepehr0Day
Version: 1.2.0
License: MIT License
Repository: https://github.com/V2RayRoot/V2Root
"""

from .v2root import V2ROOT
from .logger import (
    configure_logger, 
    get_logger, 
    debug, 
    info, 
    warning, 
    error, 
    critical, 
    logger, 
    log_function_call,
    set_level
)
from .subscription import (
    SubscriptionManager,
    Subscription,
    ConfigMetadata,
    SubscriptionError,
    FetchError,
    ParseError
)

__all__ = [
    'V2ROOT',
    'SubscriptionManager',
    'Subscription',
    'ConfigMetadata',
    
    'SubscriptionError',
    'FetchError',
    'ParseError',
    
    'logger',
    'configure_logger',
    'get_logger',
    'set_level',
    'log_function_call',
    'debug',
    'info',
    'warning',
    'error',
    'critical',
    
    '__version__',
    '__author__',
    '__email__',
]

__version__ = '1.2.0'
__author__ = 'Project V2Root, Sepehr0Day'
__license__ = 'MIT'
__email__ = 'sphrz2324@gmail.com'
__url__ = 'https://github.com/V2RayRoot/V2Root'
__description__ = 'A Python library for managing, using, and testing V2Ray configurations'