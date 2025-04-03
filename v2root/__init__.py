# v2root/__init__.py
"""
V2Root - A Python package to manage v2ray with native extensions.

This package provides a Python interface to interact with the v2ray proxy software using a custom C library (libv2root.dll).
It allows users to load configurations, start/stop v2ray, test connections, and parse VLESS strings into v2ray config files.

Author: Sepehr0Day
Version: 1.0.0
Created: April 2025
License: MIT License
Repository: https://github.com/sepehr0day/v2root (replace with actual URL if available)
Documentation: (add URL if you have docs, e.g., https://v2root.readthedocs.io/)
Contact: your.email@example.com (replace with your email)

Dependencies:
- Python 3.6+
- Windows OS (due to reliance on libv2root.dll and v2ray.exe)
- ctypes (standard library)
- urllib.request (standard library)
"""

from .v2root import V2ROOT

__all__ = ['V2ROOT']
__version__ = '1.0.0'
__author__ = 'Sepehr0Day'
__license__ = 'MIT'
__email__ = 'sphrz2324@gmail.com' 
__url__ = 'https://github.com/V2RayRoot/V2Root'
__description__ = 'A Python package to manage v2ray with native extensions'