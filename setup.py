"""Delegate legacy setuptools invocations to metadata defined in pyproject.toml.

This file intentionally contains no project metadata. Modern builds use the
PEP 517 backend configured in ``pyproject.toml``; older tooling can still run
``python setup.py`` and reach the same setuptools configuration.
"""

from setuptools import setup


setup()
