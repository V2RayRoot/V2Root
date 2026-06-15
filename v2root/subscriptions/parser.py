"""Strict subscription and share-URI metadata parsing."""

import base64
import json
from urllib.parse import unquote, urlsplit

from ..errors import ParseError
from .entities import ConfigMetadata


SUPPORTED_SCHEMES = ("vmess", "vless", "trojan", "ss", "ssr")


def _decode_base64(value: str) -> bytes:
    """Decode standard or URL-safe Base64 after restoring padding.

    Args:
        value (str): Encoded text, optionally without ``=`` padding.

    Returns:
        bytes: Decoded binary payload.

    Raises:
        ValueError: If the input cannot be decoded.
    """
    normalized = value.strip().replace("-", "+").replace("_", "/")
    normalized += "=" * (-len(normalized) % 4)
    return base64.b64decode(normalized, validate=False)


class SubscriptionParser:
    """Parse subscription documents and extract metadata from supported share URIs."""
    def parse(self, content: str, previous=None) -> list[ConfigMetadata]:
        """Parse, deduplicate, and merge one subscription document.

        Args:
            content (str): Plain-text URI lines or a Base64-encoded document.
            previous (Iterable[ConfigMetadata] | None): Existing metadata whose
                test history and tags should be preserved by exact URI match.

        Returns:
            list[ConfigMetadata]: Parsed unique configurations in source order.

        Raises:
            ParseError: If input is empty, undecodable, or contains no supported URIs.
        """
        if not isinstance(content, str) or not content.strip():
            raise ParseError("Subscription content is empty")
        lines = self._lines(content.strip())
        valid = []
        seen = set()
        previous_by_value = {item.config_string: item for item in previous or []}
        for line in lines:
            if line in seen or not line.lower().startswith(tuple(f"{s}://" for s in SUPPORTED_SCHEMES)):
                continue
            seen.add(line)
            parsed = self.parse_config(line)
            old = previous_by_value.get(line)
            if old:
                parsed.last_test_time = old.last_test_time
                parsed.last_latency = old.last_latency
                parsed.success_count = old.success_count
                parsed.failure_count = old.failure_count
                parsed.tags = list(old.tags)
            valid.append(parsed)
        if not valid:
            raise ParseError("Subscription contains no supported configurations")
        return valid

    def _lines(self, content: str) -> list[str]:
        """Extract significant lines from plain text or Base64 content.

        Args:
            content (str): Nonempty stripped subscription body.

        Returns:
            list[str]: Nonempty stripped source lines.

        Raises:
            ParseError: If a single-line body is not valid UTF-8 Base64.
        """
        direct = [line.strip() for line in content.splitlines() if line.strip()]
        if len(direct) > 1 or any("://" in line for line in direct):
            return direct
        try:
            decoded = _decode_base64(content).decode("utf-8-sig")
        except (ValueError, UnicodeDecodeError) as exc:
            raise ParseError("Subscription is neither plain text nor valid Base64") from exc
        return [line.strip() for line in decoded.splitlines() if line.strip()]

    def parse_config(self, value: str) -> ConfigMetadata:
        """Extract common metadata from one share URI.

        Args:
            value (str): Supported URI beginning with a recognized scheme.

        Returns:
            ConfigMetadata: Protocol, name, address, port, and original URI.

        Raises:
            ParseError: If a VMess payload is malformed.
            ValueError: If URI port syntax is invalid.
        """
        scheme = value.split("://", 1)[0].lower()
        if scheme == "vmess":
            return self._parse_vmess(value)
        split = urlsplit(value)
        return ConfigMetadata(
            config_string=value,
            protocol=scheme,
            name=unquote(split.fragment) or "Unnamed Config",
            address=split.hostname or "unknown",
            port=split.port or (443 if scheme in {"vless", "trojan"} else 0),
        )

    def _parse_vmess(self, value: str) -> ConfigMetadata:
        """Decode a VMess JSON payload into common metadata.

        Args:
            value (str): Complete ``vmess://`` URI.

        Returns:
            ConfigMetadata: Parsed VMess node metadata.

        Raises:
            ParseError: If Base64, JSON, or port conversion fails.
        """
        try:
            payload = json.loads(_decode_base64(value.split("://", 1)[1]))
            return ConfigMetadata(
                config_string=value,
                protocol="vmess",
                name=str(payload.get("ps") or "Unnamed Config"),
                address=str(payload.get("add") or "unknown"),
                port=int(payload.get("port") or 0),
            )
        except (ValueError, TypeError, json.JSONDecodeError) as exc:
            raise ParseError("Invalid VMess configuration") from exc
