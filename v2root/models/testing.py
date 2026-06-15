"""Models for latency and connectivity testing."""

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class LatencyResult:
    """Describe one ordered latency measurement or its per-item failure."""
    index: int
    config: str
    latency_ms: int | None
    error: str | None

    @property
    def succeeded(self) -> bool:
        """Return whether the core produced a numeric latency.

        Returns:
            bool: ``True`` when ``latency_ms`` is not ``None``.
        """
        return self.latency_ms is not None
