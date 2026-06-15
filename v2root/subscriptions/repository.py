"""Atomic JSON persistence for subscriptions."""

import json
import os
from pathlib import Path

from ..errors import PersistenceError
from .entities import Subscription


class JsonSubscriptionRepository:
    """Persist subscriptions as independent atomically replaced JSON documents."""
    def __init__(self, directory):
        """Create or open the subscription storage directory.

        Args:
            directory (str | os.PathLike[str]): Repository directory.

        Raises:
            OSError: If the directory cannot be created or resolved.
        """
        self.directory = Path(directory).expanduser().resolve()
        self.directory.mkdir(parents=True, exist_ok=True)

    def list(self) -> list[Subscription]:
        """Load all stored subscription documents in deterministic order.

        Returns:
            list[Subscription]: Valid restored subscription models.

        Raises:
            PersistenceError: If any document cannot be read, decoded, or validated.
        """
        result = []
        for path in sorted(self.directory.glob("*.json")):
            try:
                result.append(Subscription.from_dict(json.loads(path.read_text(encoding="utf-8"))))
            except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
                raise PersistenceError(f"Cannot load subscription file {path}: {exc}") from exc
        return result

    def save(self, subscription: Subscription) -> None:
        """Serialize and atomically replace one subscription document.

        Args:
            subscription (Subscription): Model to persist.

        Returns:
            None: Destination is replaced atomically when possible.

        Raises:
            PersistenceError: If temporary writing or replacement fails.
        """
        destination = self.directory / f"{subscription.id}.json"
        temporary = destination.with_suffix(".json.tmp")
        try:
            temporary.write_text(
                json.dumps(subscription.to_dict(), ensure_ascii=False, indent=2),
                encoding="utf-8",
            )
            os.replace(temporary, destination)
        except OSError as exc:
            temporary.unlink(missing_ok=True)
            raise PersistenceError(f"Cannot save subscription {subscription.name}: {exc}") from exc

    def delete(self, subscription_id: str) -> bool:
        """Delete one stored subscription document.

        Args:
            subscription_id (str): Stable source identifier used as filename.

        Returns:
            bool: Whether the destination existed before deletion.

        Raises:
            OSError: If an existing file cannot be removed.
        """
        path = self.directory / f"{subscription_id}.json"
        existed = path.exists()
        path.unlink(missing_ok=True)
        return existed
