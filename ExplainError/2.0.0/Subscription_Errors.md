# Subscription Errors

## `FetchError`

Raised when the endpoint cannot be downloaded, exceeds the configured response
limit, or cannot be decoded.

Check:

1. URL uses HTTP or HTTPS.
2. DNS and TLS are working.
3. The endpoint returns subscription content rather than an HTML login page.
4. The response is below `SubscriptionTransport.max_bytes`.

## `ParseError`

Raised when the response is empty, invalid Base64, or contains no supported
share URI.

Supported schemes:

- VMess
- VLESS
- Trojan
- Shadowsocks
- ShadowsocksR metadata

## `PersistenceError`

Raised when a JSON document cannot be loaded or atomically replaced.

Check directory permissions and available disk space. Back up a corrupted JSON
file before removing or repairing it.

The repository writes a temporary `.json.tmp` file and uses atomic replacement
to avoid partially written state.
