# Configuration Errors

## `ConfigurationError`

This exception covers parser, validator, and share-URI conversion failures.

Supported input forms include:

- `vless://`
- `vmess://`
- `trojan://`
- `ss://`
- Complete Xray-compatible JSON
- A configuration path accepted by V2Root Core

Typical causes:

- Missing credentials or remote address.
- Invalid Base64 VMess payload.
- Invalid port.
- Incomplete REALITY parameters.
- Unsupported outbound conversion.
- Invalid JSON structure.

Validate without starting:

```python
from v2root import V2RootClient

client = V2RootClient()
normalized = client.parse("vless://...")
result = client.validate(normalized)
print(result)
```

Do not print the original URI in production logs. The URI may contain reusable
credentials.
