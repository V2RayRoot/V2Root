# Core Installation and Loading Errors

## `CoreNotInstalledError`

Raised when no valid active installation exists and `auto_install=False`.

```python
from v2root import CoreManager

manager = CoreManager()
manager.install()
```

Alternatively permit first-use installation:

```python
from v2root import V2RootClient

client = V2RootClient(auto_install=True)
```

## `CoreDownloadError`

Possible causes:

- GitHub is unreachable.
- The latest release metadata is unavailable.
- The release has no ZIP matching the current OS and architecture.
- The downloaded size differs from GitHub metadata.
- The configured download size limit was exceeded.

Recommended checks:

1. Confirm access to `github.com` and `api.github.com`.
2. Inspect `CoreManager().target`.
3. Try a pinned release with `CoreManager.install("release-tag")`.
4. Do not manually rename an asset from another architecture.

## `CoreIntegrityError`

Raised before activation when:

- A ZIP contains an absolute path or `..` path traversal.
- The binary, generated header, or `SHA256SUMS` is missing.
- A binary or header digest does not match `SHA256SUMS`.
- The application attempts to remove the active release.

Delete the affected cached release and download it again. Do not disable the
integrity check.

## `CoreCompatibilityError`

Possible causes:

- A 32-bit binary is loaded into a 64-bit Python process, or the reverse.
- A Linux SO is supplied on Windows.
- A required V2Root 2.0 ABI export such as `FreeCString` is missing.
- The binary and generated header came from different releases.

Use the exact release package selected for the current `PlatformTarget`.
