# V2Root

A Python package to manage v2ray with native extensions.

V2Root provides a Python interface to interact with the v2ray proxy software using a custom C library (`libv2root.dll`). It allows users to load configurations, start/stop v2ray, test connections, and parse VLESS strings into v2ray config files.

## Features

- Load and validate v2ray configuration files
- Start and stop v2ray processes
- Test server connections with ping and VLESS-specific tests
- Parse VLESS strings into v2ray-compatible JSON configs

## Installation

Install via pip:

```bash
pip install v2root
```

## Usage
Basic example to start v2ray with a VLESS configuration:

```python
from v2root import V2ROOT

# Initialize V2ROOT
v2 = V2ROOT()

# Set a VLESS string
vless_str = "vless://your-uuid@your-server:443?security=tls&type=tcp"
v2.set_vless_string(vless_str)

# Load config and start v2ray
v2.load_config()
v2.start()

# Stop v2ray when done
v2.stop()
```

## Requirements
- Python 3.6 or higher
- Windows OS (due to dependency on libv2root.dll and v2ray.exe)
- Standard libraries: ctypes, urllib.request

## License
This project is licensed under the MIT License - see the  file for details.

## Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your changes.

## Support
If you encounter any issues or have questions, feel free to open an issue on the  <a href="https://github.com/V2RayRoot/V2Root/issues">GitHub repository</a>.