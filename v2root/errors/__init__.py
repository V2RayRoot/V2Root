"""Typed errors and human-readable diagnostics."""

from .exceptions import (
    ConfigurationError,
    CoreCompatibilityError,
    CoreDownloadError,
    CoreError,
    CoreIntegrityError,
    CoreNotInstalledError,
    FetchError,
    NativeCallError,
    ParseError,
    PersistenceError,
    SubscriptionError,
    V2RootError,
)
from .explainer import ErrorExplanation, ErrorExplainer, explain_error

__all__ = [
    "ConfigurationError",
    "CoreCompatibilityError",
    "CoreDownloadError",
    "CoreError",
    "CoreIntegrityError",
    "CoreNotInstalledError",
    "ErrorExplanation",
    "ErrorExplainer",
    "FetchError",
    "NativeCallError",
    "ParseError",
    "PersistenceError",
    "SubscriptionError",
    "V2RootError",
    "explain_error",
]
