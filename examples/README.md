# V2Root 2.0 Examples

These examples use placeholders and do not contain working credentials.

| File | Demonstrates |
| --- | --- |
| `01_quick_connect.py` | Automatic core installation and context-managed connection |
| `02_manual_core_management.py` | Install, list, update, rollback, and remove |
| `03_configuration_workflow.py` | Parse, validate, select, and convert configs |
| `04_latency_testing.py` | Raw and structured batch latency results |
| `05_runtime_metrics.py` | Status, version, traffic, and realtime speed |
| `06_subscription_management.py` | Add, update, filter, rank, and persist subscriptions |
| `07_subscription_testing.py` | Test subscriptions and save latency metadata |
| `08_logging.py` | Python SDK logging and native core logging |
| `09_error_handling.py` | Typed exceptions and structured explanations |
| `10_custom_paths.py` | Custom core cache, subscription storage, and geo assets |
| `11_legacy_facade.py` | Compatibility usage through `V2ROOT` |

Replace `vless://REPLACE_ME` and example URLs before running network examples.

```powershell
python examples/01_quick_connect.py
```

Automatic installation downloads the official platform-specific core on first
use. Set `V2ROOT_CORE_HOME` to choose a custom cache directory.
