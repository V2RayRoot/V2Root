"""Lifecycle-safe background update scheduler."""

import threading


class SubscriptionScheduler:
    """Run one update callback periodically with explicit ownership semantics."""
    def __init__(self, callback, interval):
        """Store scheduling configuration without starting a thread.

        Args:
            callback (Callable[[], object]): Operation invoked each interval.
            interval (int | float): Delay between invocations in seconds.

        Raises:
            ValueError: If ``interval`` is not positive.
        """
        if interval <= 0:
            raise ValueError("interval must be positive")
        self.callback = callback
        self.interval = interval
        self._stop = threading.Event()
        self._thread = None

    @property
    def running(self):
        """Return whether the worker thread is alive.

        Returns:
            bool: Current worker liveness.
        """
        return bool(self._thread and self._thread.is_alive())

    def start(self):
        """Start the worker unless it is already running.

        Returns:
            bool: ``True`` when a new thread is created.
        """
        if self.running:
            return False
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True, name="v2root-subscription")
        self._thread.start()
        return True

    def stop(self, timeout=5):
        """Request shutdown and join the worker.

        Args:
            timeout (int | float): Maximum join time in seconds.

        Returns:
            bool: ``True`` when no worker remains alive.
        """
        self._stop.set()
        if self._thread and self._thread is not threading.current_thread():
            self._thread.join(timeout)
        return not self.running

    def _run(self):
        """Execute callbacks after each interval until stopped.

        Returns:
            None: The loop exits when the stop event is signaled.
        """
        while not self._stop.wait(self.interval):
            self.callback()
