"""Exercise the high-level client with a deterministic in-memory native ABI."""

from v2root.models import RuntimeStatus


def test_lifecycle_and_typed_runtime_models(client):
    """Verify fluent connection, typed metrics, shutdown, and native deallocation.

    Args:
        client (tuple[V2RootClient, FakeLibrary]): Fixture containing the SDK
            and its in-memory native implementation.

    Returns:
        None: Assertions verify observable client behavior.
    """
    sdk, library = client
    sdk.connect("vless://example")
    assert sdk.runtime_status is RuntimeStatus.RUNNING
    assert sdk.version_info.code_version == 7
    assert sdk.traffic.downlink == 2
    assert sdk.speed.uplink == 3.5
    assert sdk.stop() == "STOPPED"
    assert not library.buffers


def test_detailed_latency_results(client):
    """Verify numeric and failed native measurements become typed result objects.

    Args:
        client (tuple[V2RootClient, FakeLibrary]): Deterministic client fixture.

    Returns:
        None: Assertions validate result ordering and error preservation.
    """
    sdk, _ = client
    results = sdk.test_latency_detailed(["a", "b"])
    assert results[0].succeeded and results[0].latency_ms == 42
    assert not results[1].succeeded and results[1].error == "[ERROR]bad"
