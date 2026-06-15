"""Stable exception hierarchy for programmatic error handling."""


class V2RootError(RuntimeError):
    """Base class for all operational failures raised by V2Root 2.0."""


class CoreError(V2RootError):
    """Base class for core discovery, installation, update, and loading failures."""


class CoreNotInstalledError(CoreError):
    """Indicate that no active core exists and automatic installation is disabled."""


class CoreDownloadError(CoreError):
    """Indicate that release metadata or a core archive could not be downloaded."""


class CoreIntegrityError(CoreError):
    """Indicate that an archive layout, digest, or installation state is unsafe."""


class CoreCompatibilityError(CoreError):
    """Indicate that a platform or native library does not implement the required ABI."""


class NativeCallError(V2RootError):
    """Represent an error returned by an exported V2Root Core operation."""


class ConfigurationError(NativeCallError):
    """Represent parsing, validation, or conversion failure for a proxy configuration."""


class SubscriptionError(V2RootError):
    """Base class for subscription transport, parsing, and storage failures."""


class FetchError(SubscriptionError):
    """Indicate that a subscription endpoint could not be fetched safely."""


class ParseError(SubscriptionError):
    """Indicate that subscription content or a share URI could not be parsed."""


class PersistenceError(SubscriptionError):
    """Indicate that subscription state could not be loaded or stored atomically."""
