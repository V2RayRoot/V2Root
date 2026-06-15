"""Injectable HTTP transport with bounded responses and explicit decoding."""

from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

from ..errors import FetchError


class SubscriptionTransport:
    """Fetch subscription documents with bounded memory and explicit text decoding."""
    def __init__(self, user_agent="v2root-python/2.0", max_bytes=10 * 1024 * 1024):
        """Configure subscription HTTP request limits.

        Args:
            user_agent (str): HTTP User-Agent value.
            max_bytes (int): Maximum accepted response body size.

        Raises:
            ValueError: If ``max_bytes`` is not positive.
        """
        if max_bytes <= 0:
            raise ValueError("max_bytes must be positive")
        self.user_agent = user_agent
        self.max_bytes = max_bytes

    def fetch(self, url: str, timeout=30) -> str:
        """Download and decode one subscription document.

        Args:
            url (str): Absolute HTTP(S) endpoint.
            timeout (int | float): Socket timeout in seconds.

        Returns:
            str: Response decoded as UTF-8 BOM, UTF-8, or Latin-1.

        Raises:
            FetchError: If transport fails, size is excessive, or decoding fails.
        """
        request = Request(url, headers={"User-Agent": self.user_agent, "Accept": "text/plain,*/*"})
        try:
            with urlopen(request, timeout=timeout) as response:
                content = response.read(self.max_bytes + 1)
        except (HTTPError, URLError, OSError) as exc:
            raise FetchError(f"Failed to fetch subscription: {exc}") from exc
        if len(content) > self.max_bytes:
            raise FetchError("Subscription response exceeds the configured size limit")
        for encoding in ("utf-8-sig", "utf-8", "latin-1"):
            try:
                return content.decode(encoding)
            except UnicodeDecodeError:
                continue
        raise FetchError("Subscription response uses an unsupported encoding")
