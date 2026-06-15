# Runtime and Native Operation Errors

## `NativeCallError`

The shared library loaded successfully, but an exported operation returned an
error.

Common situations:

- `start()` is called while the process-global runtime is already active.
- `traffic` or `speed` is requested while stopped.
- `stop(ignore_stopped=False)` is called while no server is running.
- A core update is attempted before stopping the runtime.
- Native JSON output is malformed or unexpectedly null.

Collect safe diagnostics:

```python
print(client.runtime_status)
print(client.version_info)
```

Lifecycle should be serialized:

```python
with V2RootClient() as client:
    client.connect("vless://...")
    # Application work
```

The context manager stops an active runtime during normal and exceptional exit.
