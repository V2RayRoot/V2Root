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

## Custom V2Ray Path

V2ROOT automatically detects the V2Ray executable in the following order:

1. **User-provided path** (via constructor parameter)
2. **Environment variable** `V2RAY_PATH`
3. **Bundled executable** (included in the package)
4. **System-installed V2Ray** (Linux: `/usr/bin/v2ray`, `/usr/local/bin/v2ray`, etc.; Windows: Program Files)

### Usage Examples

#### Using Constructor Parameter

```python
from v2root import V2ROOT

# Specify custom V2Ray path
proxy = V2ROOT(http_port=2300, socks_port=2301, v2ray_path="/custom/path/to/v2ray")
```

#### Using Environment Variable

**Linux/macOS:**
```bash
export V2RAY_PATH=/custom/path/to/v2ray
python your_script.py
```

**Windows:**
```cmd
set V2RAY_PATH=C:\custom\path\to\v2ray.exe
python your_script.py
```

#### Auto-Detection (Default)

```python
from v2root import V2ROOT

# Uses auto-detection (bundled or system-installed V2Ray)
proxy = V2ROOT(http_port=2300, socks_port=2301)
```

### Installation Notes

**Linux:**
```bash
# Install V2Ray via package manager
sudo apt install v2ray  # Debian/Ubuntu
sudo yum install v2ray  # CentOS/RHEL

# Or download manually
wget https://github.com/v2fly/v2ray-core/releases/latest/download/v2ray-linux-64.zip
unzip v2ray-linux-64.zip -d /usr/local/bin/
chmod +x /usr/local/bin/v2ray
```

**Windows:**
- Download from [V2Ray Releases](https://github.com/v2fly/v2ray-core/releases)
- Extract to `C:\Program Files\V2Ray\` or any directory
- Use `v2ray_path` parameter or `V2RAY_PATH` environment variable

## Requirements
- Python 3.6 or higher
- V2Ray executable (v2ray.exe on Windows, v2ray on Linux)
- Windows or Linux OS
- Standard libraries: ctypes, colorama

## Documentation
Detailed documentation, including installation instructions, usage examples, and supported configuration options, is available at:
<a href="https://v2root.readthedocs.io/en/latest/">Read the Docs</a>

## License
This project is licensed under the MIT License - see the file for details.

## Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your changes. See the <a href="https://v2root.readthedocs.io/en/latest/contributing.html"> Contributing Guide</a> for more details.

## What's New
- Fixed Shadowsocks parser
- Resolved service execution issues with the latest V2Ray version on Linux
- Updated Explain Error section for better error handling and user friendly troubleshooting

## Support
If you encounter any issues or have questions, feel free to open an issue on the <a href="https://github.com/V2RayRoot/V2Root/issues"> GitHub repository</a> or join our <a href="https://t.me/DevSepehr">Support Channel</a>.

# V2ROOT - V2Ray Proxy Management Library

V2ROOT is a Python library for managing V2Ray proxy connections on Windows and Linux platforms. It provides a simple interface to configure, start, stop, and test V2Ray connections.

## Platform Requirements

### Windows

**V2Ray executable is REQUIRED and must be provided by the user.**

1. Download V2Ray from the official repository:
   - https://github.com/v2fly/v2ray-core/releases
   - Download the Windows version (e.g., `v2ray-windows-64.zip`)

2. Extract the archive to a directory of your choice (e.g., `C:\V2Ray\`)

3. Provide the path to `v2ray.exe` when initializing V2ROOT:

```python
from v2root import V2ROOT

# Windows - v2ray_path is REQUIRED
proxy = V2ROOT(
    http_port=2300,
    socks_port=2301,
    v2ray_path=r"C:\V2Ray\v2ray.exe"  # REQUIRED on Windows
)
```

### Linux

**V2Ray must be installed via package manager. Custom paths are NOT supported.**

Install V2Ray using your distribution's package manager:

#### Debian/Ubuntu:
```bash
sudo apt update
sudo apt install v2ray
```

#### Fedora/RHEL/CentOS:
```bash
sudo dnf install v2ray
# or
sudo yum install v2ray
```

#### Arch Linux:
```bash
sudo pacman -S v2ray
```

#### Manual Installation (if not available in package manager):
```bash
bash <(curl -L https://raw.githubusercontent.com/v2fly/fhs-install-v2ray/master/install-release.sh)
```

Verify installation:
```bash
v2ray version
```

Then initialize V2ROOT (no path needed):

```python
from v2root import V2ROOT

# Linux - v2ray_path is ignored, uses system-installed V2Ray
proxy = V2ROOT(
    http_port=2300,
    socks_port=2301
    # v2ray_path is ignored on Linux
)
```

## Installation

```bash
pip install v2root
```

## Usage Examples

### Windows Example

```python
from v2root import V2ROOT

# Initialize with V2Ray path (REQUIRED on Windows)
proxy = V2ROOT(
    http_port=2300,
    socks_port=2301,
    v2ray_path=r"C:\Path\To\v2ray.exe"  # Change this to your actual path
)

# Set configuration
config = "vless://uuid@server.com:443?encryption=none&security=tls&type=ws&host=server.com&path=/path"
proxy.set_config_string(config)

# Start proxy
proxy.start()

# Stop proxy
proxy.stop()
```

### Linux Example

```bash
# First, install V2Ray via package manager
sudo apt install v2ray  # Debian/Ubuntu
```

```python
from v2root import V2ROOT

# Initialize (no v2ray_path needed on Linux)
proxy = V2ROOT(http_port=2300, socks_port=2301)

# Set configuration
config = "vless://uuid@server.com:443?encryption=none&security=tls&type=ws&host=server.com&path=/path"
proxy.set_config_string(config)

# Start proxy
proxy.start()

# Stop proxy
proxy.stop()
```

## API Reference

### Initialization

```python
V2ROOT(http_port=2300, socks_port=2301, v2ray_path=None)
```

**Parameters:**
- `http_port` (int): HTTP proxy port (default: 2300)
- `socks_port` (int): SOCKS proxy port (default: 2301)
- `v2ray_path` (str): Path to V2Ray executable
  - **Windows**: REQUIRED - Must point to `v2ray.exe`
  - **Linux**: Ignored - System-installed V2Ray is used

**Raises:**
- `RuntimeError`: On Windows if `v2ray_path` is not provided or invalid
- `RuntimeError`: On Linux if V2Ray is not installed via package manager
- `FileNotFoundError`: If V2Ray executable is not found (Windows)
- `ValueError`: If ports are invalid

### Methods

- `set_config_string(config_str)` - Parse and set V2Ray configuration
- `start()` - Start V2Ray proxy service
- `stop()` - Stop V2Ray proxy service
- `test_connection(config_str)` - Test configuration and return latency
- `ping_server(address, port)` - Ping server to measure latency
- `reset_network_proxy()` - Reset system proxy settings

## Supported Protocols

- VLESS (with TLS, WebSocket, gRPC, etc.)
- VMess (with TLS, WebSocket, HTTP/2, etc.)
- Shadowsocks (with plugins)

## Error Handling

The library provides detailed error messages for common issues:

### Windows Errors

```python
# Error if v2ray_path is not provided
RuntimeError: v2ray_path is required on Windows!
Please provide the path to v2ray.exe when initializing V2ROOT:
  proxy = V2ROOT(v2ray_path='C:\\path\\to\\v2ray.exe')

# Error if v2ray.exe is not found
FileNotFoundError: V2Ray executable not found at: C:\path\to\v2ray.exe
```

### Linux Errors

```python
# Error if V2Ray is not installed
RuntimeError: V2Ray is not installed on this system!
Installation instructions:
  Debian/Ubuntu:
    sudo apt update
    sudo apt install v2ray
```

## Troubleshooting

### Windows

1. **"v2ray_path is required"**
   - You must provide the path to v2ray.exe
   - Download V2Ray and extract it
   - Use the full path: `V2ROOT(v2ray_path=r"C:\V2Ray\v2ray.exe")`

2. **"V2Ray executable not found"**
   - Check that the path is correct
   - Ensure v2ray.exe exists at the specified location
   - Use raw string (r"...") or escape backslashes

### Linux

1. **"V2Ray is not installed"**
   - Install V2Ray via package manager: `sudo apt install v2ray`
   - Verify installation: `v2ray version`

2. **Permission errors**
   - Run with sudo if needed: `sudo python3 your_script.py`
   - Or fix permissions: `sudo chmod +x /usr/bin/v2ray`

## License

MIT License

## Support

- GitHub: https://github.com/V2RayRoot/V2Root
- Issues: https://github.com/V2RayRoot/V2Root/issues
- Telegram: @Sepehr0Day
