"""Small injectable HTTP transport used by the core manager."""

import json
from contextlib import closing
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

from ..errors import CoreDownloadError


class HttpTransport:
    """Provide bounded, injectable HTTP operations for GitHub release management."""
    def __init__(self, timeout=30, user_agent="v2root-python/2.0"):
        """Configure HTTP behavior used for release operations.

        Args:
            timeout (int | float): Socket timeout in seconds.
            user_agent (str): User-Agent sent to GitHub and asset hosts.
        """
        self.timeout = timeout
        self.user_agent = user_agent

    def get_json(self, url):
        """Fetch and decode one GitHub API JSON document.

        Args:
            url (str): HTTPS GitHub API endpoint.

        Returns:
            object: JSON-decoded response.

        Raises:
            CoreDownloadError: If transport, status, decoding, or JSON parsing fails.
        """
        request = Request(
            url,
            headers={
                "Accept": "application/vnd.github+json",
                "User-Agent": self.user_agent,
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        try:
            with closing(urlopen(request, timeout=self.timeout)) as response:
                return json.loads(response.read().decode("utf-8"))
        except (HTTPError, URLError, OSError, ValueError) as exc:
            raise CoreDownloadError(f"Failed to fetch release metadata: {exc}") from exc

    def download(self, url, destination, expected_size=None, max_size=300_000_000):
        """Stream a release asset to disk with bounded size validation.

        Args:
            url (str): Asset download URL.
            destination (str | os.PathLike[str]): Output file path.
            expected_size (int | None): Exact expected byte count from metadata.
            max_size (int): Hard upper download limit in bytes.

        Returns:
            pathlib.Path: Completed destination path.

        Raises:
            CoreDownloadError: If transfer fails, exceeds limits, or has an
                unexpected final size.
        """
        destination = Path(destination)
        request = Request(url, headers={"User-Agent": self.user_agent})
        received = 0
        try:
            with closing(urlopen(request, timeout=self.timeout)) as response:
                content_length = response.headers.get("Content-Length")
                if content_length and int(content_length) > max_size:
                    raise CoreDownloadError("Core archive exceeds the download limit")
                with destination.open("wb") as output:
                    while True:
                        chunk = response.read(1024 * 1024)
                        if not chunk:
                            break
                        received += len(chunk)
                        if received > max_size:
                            raise CoreDownloadError(
                                "Core archive exceeds the download limit"
                            )
                        output.write(chunk)
        except (HTTPError, URLError, OSError) as exc:
            destination.unlink(missing_ok=True)
            raise CoreDownloadError(f"Failed to download core archive: {exc}") from exc
        if expected_size and received != expected_size:
            destination.unlink(missing_ok=True)
            raise CoreDownloadError(
                f"Downloaded size mismatch: expected {expected_size}, got {received}"
            )
        return destination
