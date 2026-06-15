"""Verify that every documented example remains importable without side effects."""

import importlib.util
from pathlib import Path

import pytest


EXAMPLES = sorted(Path("examples").glob("*.py"))


@pytest.mark.parametrize("path", EXAMPLES, ids=lambda path: path.name)
def test_example_imports_without_running_main(path: Path) -> None:
    """Import one example module without executing its guarded entry point.

    Args:
        path (pathlib.Path): Example script selected by pytest.

    Returns:
        None: Import completion proves syntax and public imports are valid.

    Raises:
        ImportError: If the example references an unavailable SDK symbol.
    """
    spec = importlib.util.spec_from_file_location(f"example_{path.stem}", path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
