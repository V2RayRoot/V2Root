"""Verify structured explanations for the V2Root 2.0 exception hierarchy."""

from v2root import explain_error
from v2root.errors import CoreIntegrityError, ParseError


def test_explains_core_integrity_failures() -> None:
    """Ensure integrity failures produce stable, retryable remediation guidance.

    Returns:
        None: Assertions validate the explanation value object.
    """
    explanation = explain_error(CoreIntegrityError("SHA-256 mismatch"))
    assert explanation.code == "CORE_INTEGRITY_FAILED"
    assert explanation.retryable
    assert explanation.actions


def test_explains_subscription_parse_failures() -> None:
    """Ensure subscription parse errors are classified independently.

    Returns:
        None: Assertions validate classification and formatted output.
    """
    explanation = explain_error(ParseError("No supported configurations"))
    assert explanation.code == "SUBSCRIPTION_PARSE_FAILED"
    assert "Probable causes:" in explanation.format()


def test_unknown_values_receive_generic_guidance() -> None:
    """Ensure values outside the 2.0 exception hierarchy remain unclassified.

    Returns:
        None: The test confirms generic fallback behavior.
    """
    explanation = explain_error(-7)
    assert explanation.code == "UNEXPECTED_ERROR"
