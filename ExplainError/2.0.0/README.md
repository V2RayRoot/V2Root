# V2Root 2.0.0 Error Guide

V2Root 2.0 uses typed Python exceptions instead of numeric error codes. Catch
the narrowest exception that your application can handle and use
`v2root.explain_error()` to produce structured user-facing guidance.

```python
from v2root import V2RootClient, V2RootError, explain_error

try:
    client = V2RootClient()
except V2RootError as error:
    explanation = explain_error(error)
    print(explanation.code)
    print(explanation.format())
```

## Categories

| Exception | Meaning | Retry |
| --- | --- | --- |
| `CoreNotInstalledError` | No active core and auto-install is disabled | After installation |
| `CoreDownloadError` | Release metadata or archive download failed | Usually |
| `CoreIntegrityError` | ZIP structure or SHA-256 verification failed | After clean download |
| `CoreCompatibilityError` | OS, architecture, binary, or ABI mismatch | No |
| `ConfigurationError` | Share URI, JSON, validation, or conversion failed | After correction |
| `NativeCallError` | Embedded runtime rejected an operation | Depends on state |
| `FetchError` | Subscription endpoint could not be fetched | Usually |
| `ParseError` | Subscription response contains no usable configs | After correction |
| `PersistenceError` | Subscription JSON state could not be read or written | After storage repair |

## Security

- Never include complete share URIs in issue reports.
- Remove UUIDs, passwords, private keys, and query parameters from logs.
- Do not bypass SHA-256 verification.
- Download V2Root Core only through the official release repository.
- Include `client.version_info`, operating system, architecture, and the
  explanation code in sanitized reports.

Historical 1.x numeric-code documentation remains in `ExplainError/1.1.2`.
