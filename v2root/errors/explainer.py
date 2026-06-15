"""Translate V2Root 2.0 exceptions into structured operational guidance."""

from dataclasses import dataclass

from .exceptions import (
    ConfigurationError,
    CoreCompatibilityError,
    CoreDownloadError,
    CoreIntegrityError,
    CoreNotInstalledError,
    FetchError,
    NativeCallError,
    ParseError,
    PersistenceError,
    SubscriptionError,
)


@dataclass(frozen=True, slots=True)
class ErrorExplanation:
    """A presentation-neutral diagnosis suitable for logs, CLIs, and user interfaces."""

    code: str
    title: str
    summary: str
    probable_causes: tuple[str, ...]
    actions: tuple[str, ...]
    retryable: bool

    def format(self) -> str:
        """Render the diagnosis as readable plain text.

        Returns:
            str: Title, summary, probable causes, and numbered actions.
        """
        causes = "\n".join(f"- {cause}" for cause in self.probable_causes)
        actions = "\n".join(
            f"{index}. {action}" for index, action in enumerate(self.actions, 1)
        )
        return (
            f"{self.title}\n{self.summary}\n\n"
            f"Probable causes:\n{causes}\n\nRecommended actions:\n{actions}"
        )


class ErrorExplainer:
    """Map the V2Root 2.0 exception hierarchy to actionable, sanitized guidance."""

    def explain(self, error: BaseException) -> ErrorExplanation:
        """Return a diagnosis for a V2Root 2.0 exception.

        Args:
            error (BaseException): Exception raised by the SDK or an unexpected
                dependency failure.

        Returns:
            ErrorExplanation: Stable code, user-facing text, causes, actions,
                and retryability.

        Notes:
            Values outside the V2Root 2.0 exception hierarchy receive the
            generic ``UNEXPECTED_ERROR`` classification.
        """
        mappings = (
            (
                CoreNotInstalledError,
                "CORE_NOT_INSTALLED",
                "V2Root Core is not installed",
                ("Automatic installation is disabled.", "The active installation manifest is missing."),
                ("Enable automatic installation.", "Install a release with CoreManager.install()."),
                True,
            ),
            (
                CoreDownloadError,
                "CORE_DOWNLOAD_FAILED",
                "V2Root Core could not be downloaded",
                ("The release API is unavailable.", "The selected release has no compatible asset."),
                ("Check network access to GitHub.", "Retry or pin a known compatible release."),
                True,
            ),
            (
                CoreIntegrityError,
                "CORE_INTEGRITY_FAILED",
                "V2Root Core failed integrity verification",
                ("The archive is incomplete.", "A checksum or archive path is invalid."),
                ("Remove the affected cached release.", "Download it again from the official repository."),
                True,
            ),
            (
                CoreCompatibilityError,
                "CORE_INCOMPATIBLE",
                "The native core is incompatible with this SDK",
                ("The process architecture does not match.", "A required V2Root 2.0 ABI symbol is missing."),
                ("Install the correct platform package.", "Keep the generated header and binary from the same release."),
                False,
            ),
            (
                ConfigurationError,
                "CONFIGURATION_INVALID",
                "The proxy configuration could not be processed",
                ("The share URI is malformed.", "The JSON configuration is unsupported by the active core."),
                ("Validate the source without exposing credentials.", "Inspect the original native validation message."),
                False,
            ),
            (
                ParseError,
                "SUBSCRIPTION_PARSE_FAILED",
                "Subscription content could not be parsed",
                ("The response is empty or malformed.", "No supported configuration scheme was found."),
                ("Inspect the subscription response format.", "Confirm that it contains supported share URIs."),
                False,
            ),
            (
                FetchError,
                "SUBSCRIPTION_FETCH_FAILED",
                "The subscription could not be downloaded",
                ("The endpoint is unavailable.", "The response exceeded the configured size limit."),
                ("Verify the subscription URL.", "Retry with a suitable timeout."),
                True,
            ),
            (
                PersistenceError,
                "SUBSCRIPTION_STORAGE_FAILED",
                "Subscription state could not be persisted",
                ("The storage directory is not writable.", "A stored JSON document is corrupted."),
                ("Check directory permissions.", "Back up and repair the affected subscription file."),
                False,
            ),
            (
                NativeCallError,
                "NATIVE_CALL_FAILED",
                "V2Root Core rejected an operation",
                ("The runtime is in an incompatible state.", "The native operation returned an error."),
                ("Record the active core version and status.", "Retry only after correcting the reported condition."),
                False,
            ),
            (
                SubscriptionError,
                "SUBSCRIPTION_FAILED",
                "A subscription operation failed",
                ("A subscription component rejected the operation.",),
                ("Inspect the original exception and subscription state.",),
                False,
            ),
        )
        for exception_type, code, title, causes, actions, retryable in mappings:
            if isinstance(error, exception_type):
                return ErrorExplanation(
                    code, title, str(error), causes, actions, retryable
                )
        return ErrorExplanation(
            "UNEXPECTED_ERROR",
            "An unexpected V2Root 2.0 error occurred",
            str(error),
            ("An unclassified dependency or application error occurred.",),
            ("Inspect the traceback.", "Report a sanitized reproduction if the issue persists."),
            False,
        )


def explain_error(error: BaseException) -> ErrorExplanation:
    """Explain one V2Root 2.0 exception using the default catalog.

    Args:
        error (BaseException): Failure to classify.

    Returns:
        ErrorExplanation: Presentation-neutral diagnosis.
    """
    return ErrorExplainer().explain(error)
