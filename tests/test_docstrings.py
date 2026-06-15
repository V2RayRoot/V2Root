"""Enforce repository-wide documentation coverage for Python source objects."""

import ast
from pathlib import Path

import pytest


ROOTS = (Path("v2root"), Path("tests"), Path("examples"))


def _python_files() -> list[Path]:
    """Collect tracked source and test modules subject to documentation policy.

    Returns:
        list[pathlib.Path]: Sorted Python files excluding generated caches.
    """
    return sorted(
        path
        for root in ROOTS
        for path in root.rglob("*.py")
        if "__pycache__" not in path.parts
    )


@pytest.mark.parametrize("path", _python_files(), ids=str)
def test_every_python_object_has_a_docstring(path: Path) -> None:
    """Require module, class, function, and method docstrings in every Python file.

    Args:
        path (pathlib.Path): Python module selected by pytest parametrization.

    Returns:
        None: The test fails with exact object names and line numbers.
    """
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    missing = []
    if not ast.get_docstring(tree):
        missing.append("module:1")
    for node in ast.walk(tree):
        if isinstance(node, (ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)):
            if not ast.get_docstring(node):
                missing.append(f"{node.name}:{node.lineno}")
    assert not missing, f"{path} missing docstrings: {', '.join(missing)}"


@pytest.mark.parametrize(
    "path",
    [
        path
        for path in _python_files()
        if "v2root" in path.parts or "examples" in path.parts
    ],
    ids=str,
)
def test_production_function_docstrings_describe_contracts(path: Path) -> None:
    """Require argument and return sections for production callables.

    Args:
        path (pathlib.Path): Production Python module selected by pytest.

    Returns:
        None: Failures identify callable names and missing contract sections.

    Notes:
        Constructors are exempt from ``Returns`` because Python constructors
        always return ``None``. Context-manager ``__exit__`` methods are also
        exempt because their suppression behavior is documented in prose.
    """
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    failures = []
    for node in ast.walk(tree):
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        docstring = ast.get_docstring(node) or ""
        parameters = [
            argument.arg
            for argument in (*node.args.args, *node.args.kwonlyargs)
            if argument.arg not in {"self", "cls"}
        ]
        if parameters and "Args:" not in docstring:
            failures.append(f"{node.name}:{node.lineno} missing Args")
        if node.name not in {"__init__", "__exit__"} and "Returns:" not in docstring:
            failures.append(f"{node.name}:{node.lineno} missing Returns")
    assert not failures, f"{path} incomplete contracts: {', '.join(failures)}"
