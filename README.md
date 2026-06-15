# V2Root Python SDK 2.0

A managed, modular Python 3.10+ SDK for
[V2Root Core](https://github.com/v2RayRoot/V2Root-Core).

The package contains no DLL, SO, C source, or external Xray executable. On first
use it downloads the correct official V2Root Core release for the current
platform, verifies the release archive, and installs it in an application cache.

## Architecture

- `v2root.core.CoreManager`: discovery, install, update, activation and rollback
- `v2root.native.NativeCore`: strict `ctypes` ABI and native memory ownership
- `v2root.client.V2RootClient`: configuration, lifecycle, latency and metrics
- `v2root.V2ROOT`: compatibility facade for applications written against 1.x

## Automatic Installation

```python
from v2root import V2RootClient

client = V2RootClient()  # Downloads the latest compatible core when required.
client.connect("vless://...")
print(client.status)
print(client.traffic)
client.stop()
```

Structured batch testing and subscription integration:

```python
from v2root import SubscriptionManager, V2RootClient

subscriptions = SubscriptionManager()
source = subscriptions.add_subscription("https://example.com/subscription")

with V2RootClient() as client:
    results = client.test_subscription(subscriptions, source.id)
    best = subscriptions.best_config(max_latency=500)
    if best:
        client.connect(best.config_string)
```

All low-level failures use typed exceptions. `explain_error(error)` returns a
structured explanation with corrective actions. Legacy numeric errors `-1`
through `-7` remain documented and supported by the explainer.

The default cache is:

- Windows: `%LOCALAPPDATA%\V2Root\core`
- Linux: `$XDG_CACHE_HOME/v2root/core` or `~/.cache/v2root/core`

Set `V2ROOT_CORE_HOME` to override it.

## Core Management

```python
from v2root import CoreManager

manager = CoreManager()
installed = manager.ensure()
update = manager.update()
versions = manager.installed()
previous = manager.rollback()
```

Pin a release or disable network installation:

```python
manager.install("v2root-v26.6.15")
client = V2RootClient(auto_install=False)
```

An explicit development build can be loaded without installing it. The
``xray-*`` basename in this example is the ABI asset name currently published
by V2Root Core; it does not refer to a separately installed Xray executable:

```python
client = V2RootClient(core_path="/path/to/xray-linux-amd64.so")
```

## Installation Security

The manager:

1. Uses the official GitHub Releases API and release asset URL.
2. Selects the exact OS and architecture package.
3. Enforces download size limits.
4. Rejects unsafe ZIP paths.
5. Requires the binary, generated header, and `SHA256SUMS`.
6. Verifies the binary and header SHA-256 digests.
7. Installs into a temporary directory and activates atomically.
8. Retains older releases for rollback.

## License

MIT License. See `LICENSE`.
