# V2Root

A Python package to manage V2Ray proxy configurations with native extensions.

V2Root provides a Python interface to interact with the V2Ray proxy software using a custom C library (`libv2root.dll` on Windows, `libv2root.so` on Linux). It allows users to load configurations, start/stop V2Ray, test connections, and parse VLESS, VMess, and Shadowsocks strings into V2Ray-compatible config files.

## Features

- Load and validate V2Ray configuration files
- Start and stop V2Ray processes
- Test server connections with ping and protocol-specific tests
- Parse VLESS, VMess, and Shadowsocks strings into V2Ray-compatible JSON configs
- Cross-platform support for Windows and Linux
- Comprehensive documentation with examples and troubleshooting

## Installation

Install via pip:

```bash
pip install v2root
```

## Usage
Basic example to start V2Ray with a VLESS configuration:

```python
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
```

## Requirements
- Python 3.6 or higher
- V2Ray executable (v2ray.exe on Windows, v2ray on Linux)
- Windows or Linux OS
- Standard libraries: ctypes, urllib.request, colorama

## Documentation
Detailed documentation, including installation instructions, usage examples, and supported configuration options, is available at:
<a href="https://v2root.readthedocs.io/en/latest/">Read the Docs</a>

## License
This project is licensed under the MIT License - see the file for details.

## Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your changes. See the <a href="https://v2root.readthedocs.io/en/latest/contributing.html"> Contributing Guide</a> for more details.

## What's New
- Added support for VMess and Shadowsocks protocols
- Introduced Linux support with libv2root.so
- Enhanced documentation with quick start guide, advanced examples, and troubleshooting

## Support
If you encounter any issues or have questions, feel free to open an issue on the <a href="https://github.com/V2RayRoot/V2Root/issues"> GitHub repository</a> or join our <a href="https://t.me/DevSepehr">Support Channel</a>.
