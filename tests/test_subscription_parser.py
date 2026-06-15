"""Test subscription document decoding and protocol metadata extraction."""

import base64
import json

from v2root.subscriptions import SubscriptionParser


def test_plain_base64_deduplication_and_ipv6():
    """Verify Base64 documents deduplicate entries and preserve IPv6 hosts.

    Returns:
        None: Assertions validate parsed metadata.
    """
    parser = SubscriptionParser()
    uri = "vless://id@[2001:db8::1]:443#IPv6"
    encoded = base64.b64encode(f"{uri}\n{uri}\n".encode()).decode()
    result = parser.parse(encoded)
    assert len(result) == 1
    assert result[0].address == "2001:db8::1"
    assert result[0].port == 443


def test_vmess_metadata():
    """Verify VMess JSON fields map to name, address, and integer port metadata.

    Returns:
        None: Assertions validate the decoded VMess model.
    """
    payload = base64.b64encode(
        json.dumps({"ps": "Node", "add": "example.com", "port": "8443"}).encode()
    ).decode()
    result = SubscriptionParser().parse_config("vmess://" + payload)
    assert (result.name, result.address, result.port) == ("Node", "example.com", 8443)
