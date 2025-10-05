# V2Root

Python toolkit for managing V2Ray configurations via native extensions.

## Installation

```bash
pip install v2root
```

## Quick Start

```python
from v2root import V2ROOT

proxy = V2ROOT(http_port=2300, socks_port=2301)
proxy.set_config_string("vless://example")
proxy.start()
# ...
proxy.stop()
```

## Documentation

Full feature list, platform requirements, advanced usage, and troubleshooting are documented at [v2root.readthedocs.io](https://v2root.readthedocs.io/en/latest/).

## Contributing

Fork the repository and open a pull request. Details: [Contributing Guide](https://v2root.readthedocs.io/en/latest/contributing.html).

## License

MIT License — see `LICENSE`.

## Support

Issues: <https://github.com/V2RayRoot/V2Root/issues>  
Telegram: <https://t.me/DevSepehr>
