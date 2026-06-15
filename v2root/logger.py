"""Configure and expose the package logging API without import-time file I/O.

The module provides a shared ``v2root`` logger, convenience functions, and a
decorator for tracing function calls. Importing the package installs only a
``NullHandler`` so applications remain in control of output destinations.
Calling :func:`configure_logger` replaces package-owned handlers explicitly.
"""

import functools
import logging
import os
import platform
from logging.handlers import RotatingFileHandler
from pathlib import Path
from typing import Callable, ParamSpec, TypeVar

try:
    from colorama import Fore, Style, init

    init(autoreset=True)
    _HAS_COLOR = True
except ImportError:
    _HAS_COLOR = False


P = ParamSpec("P")
R = TypeVar("R")


class _ColoredFormatter(logging.Formatter):
    """Apply terminal colors to a copied level name without mutating log records."""

    _COLORS = {
        logging.DEBUG: Fore.CYAN if _HAS_COLOR else "",
        logging.INFO: Fore.GREEN if _HAS_COLOR else "",
        logging.WARNING: Fore.YELLOW if _HAS_COLOR else "",
        logging.ERROR: Fore.RED if _HAS_COLOR else "",
        logging.CRITICAL: (Fore.RED + Style.BRIGHT) if _HAS_COLOR else "",
    }

    def format(self, record: logging.LogRecord) -> str:
        """Format one record with a colored level name.

        Args:
            record (logging.LogRecord): Record emitted by Python logging.

        Returns:
            str: Fully formatted console line.

        Notes:
            A shallow record copy is used so other handlers receive the
            original unmodified ``levelname``.
        """
        copy = logging.makeLogRecord(record.__dict__)
        color = self._COLORS.get(copy.levelno, "")
        if color:
            copy.levelname = f"{color}{copy.levelname}{Style.RESET_ALL}"
        return super().format(copy)


class V2RootLogger:
    """Manage handlers and convenience methods for the shared package logger.

    Args:
        name (str): Base logger name. Child loggers inherit its handlers.

    Notes:
        The wrapper does not create files during construction. Applications
        must call :meth:`configure` or :func:`configure_logger` explicitly.
    """

    DEBUG = logging.DEBUG
    INFO = logging.INFO
    WARNING = logging.WARNING
    ERROR = logging.ERROR
    CRITICAL = logging.CRITICAL

    def __init__(self, name: str = "v2root"):
        """Create a wrapper around one named Python logger.

        Args:
            name (str): Logger hierarchy root.

        Returns:
            None: The new wrapper stores the configured logger.
        """
        self.logger = logging.getLogger(name)
        self.logger.addHandler(logging.NullHandler())
        self.logger.propagate = False
        self.log_file: Path | None = None

    def configure(
        self,
        *,
        log_level: int = logging.INFO,
        log_to_file: bool = False,
        log_to_console: bool = True,
        log_dir: str | os.PathLike[str] | None = None,
        max_file_size: int = 5 * 1024 * 1024,
        backup_count: int = 3,
    ) -> "V2RootLogger":
        """Replace package handlers with explicit console and file outputs.

        Args:
            log_level (int): Standard logging threshold such as ``logging.INFO``.
            log_to_file (bool): Create a rotating UTF-8 log file when true.
            log_to_console (bool): Emit formatted records to stderr when true.
            log_dir (str | os.PathLike[str] | None): File logging directory.
                Platform-specific user storage is used when omitted.
            max_file_size (int): Rotation threshold in bytes.
            backup_count (int): Number of rotated files to retain.

        Returns:
            V2RootLogger: This configured wrapper for fluent setup.

        Raises:
            ValueError: If size or backup values are negative.
            OSError: If the log directory or file cannot be created.
        """
        if max_file_size < 0 or backup_count < 0:
            raise ValueError("max_file_size and backup_count must be nonnegative")
        for handler in list(self.logger.handlers):
            self.logger.removeHandler(handler)
            handler.close()
        self.logger.setLevel(log_level)
        if log_to_console:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            console.setFormatter(
                _ColoredFormatter(
                    "%(asctime)s [%(levelname)s] %(message)s", datefmt="%H:%M:%S"
                )
            )
            self.logger.addHandler(console)
        if log_to_file:
            directory = Path(log_dir) if log_dir else _default_log_directory()
            directory = directory.expanduser().resolve()
            directory.mkdir(parents=True, exist_ok=True)
            self.log_file = directory / "v2root.log"
            file_handler = RotatingFileHandler(
                self.log_file,
                maxBytes=max_file_size,
                backupCount=backup_count,
                encoding="utf-8",
            )
            file_handler.setLevel(log_level)
            file_handler.setFormatter(
                logging.Formatter(
                    "%(asctime)s [%(levelname)s] "
                    "%(name)s:%(funcName)s:%(lineno)d - %(message)s"
                )
            )
            self.logger.addHandler(file_handler)
        if not self.logger.handlers:
            self.logger.addHandler(logging.NullHandler())
        return self

    def get_logger(self, name: str | None = None) -> logging.Logger:
        """Return the package logger or one of its named descendants.

        Args:
            name (str | None): Optional suffix such as ``core.manager``.

        Returns:
            logging.Logger: Shared root logger or ``v2root.<name>`` child.
        """
        return logging.getLogger(f"{self.logger.name}.{name}") if name else self.logger

    def debug(self, msg, *args, **kwargs) -> None:
        """Emit a DEBUG record.

        Args:
            msg (object): Message or formatting template.
            *args: Values interpolated by Python logging.
            **kwargs: Standard ``logging.Logger.debug`` options.

        Returns:
            None: The record is delegated to the shared logger.
        """
        self.logger.debug(msg, *args, **kwargs)

    def info(self, msg, *args, **kwargs) -> None:
        """Emit an INFO record.

        Args:
            msg (object): Message or formatting template.
            *args: Values interpolated by Python logging.
            **kwargs: Standard ``logging.Logger.info`` options.

        Returns:
            None: The record is delegated to the shared logger.
        """
        self.logger.info(msg, *args, **kwargs)

    def warning(self, msg, *args, **kwargs) -> None:
        """Emit a WARNING record.

        Args:
            msg (object): Message or formatting template.
            *args: Values interpolated by Python logging.
            **kwargs: Standard ``logging.Logger.warning`` options.

        Returns:
            None: The record is delegated to the shared logger.
        """
        self.logger.warning(msg, *args, **kwargs)

    def error(self, msg, *args, **kwargs) -> None:
        """Emit an ERROR record.

        Args:
            msg (object): Message or formatting template.
            *args: Values interpolated by Python logging.
            **kwargs: Standard ``logging.Logger.error`` options.

        Returns:
            None: The record is delegated to the shared logger.
        """
        self.logger.error(msg, *args, **kwargs)

    def critical(self, msg, *args, **kwargs) -> None:
        """Emit a CRITICAL record.

        Args:
            msg (object): Message or formatting template.
            *args: Values interpolated by Python logging.
            **kwargs: Standard ``logging.Logger.critical`` options.

        Returns:
            None: The record is delegated to the shared logger.
        """
        self.logger.critical(msg, *args, **kwargs)

    def exception(self, msg, *args, **kwargs) -> None:
        """Emit an ERROR record with the active traceback.

        Args:
            msg (object): Message or formatting template.
            *args: Values interpolated by Python logging.
            **kwargs: Standard ``logging.Logger.exception`` options.

        Returns:
            None: The record is delegated to the shared logger.
        """
        self.logger.exception(msg, *args, **kwargs)

    def set_level(self, level: int) -> None:
        """Set the threshold on the root logger and every owned handler.

        Args:
            level (int): Standard numeric Python logging level.

        Returns:
            None: Logger and handler levels are updated in place.
        """
        self.logger.setLevel(level)
        for handler in self.logger.handlers:
            handler.setLevel(level)

    def log_function_call(
        self,
        func: Callable[P, R] | None = None,
        *,
        log_args: bool = False,
        log_result: bool = False,
        level: int = logging.DEBUG,
    ):
        """Create a decorator that records function entry, exit, and failures.

        Args:
            func (Callable[P, R] | None): Function when used as ``@decorator``.
            log_args (bool): Include bounded argument representations.
            log_result (bool): Include a bounded result representation.
            level (int): Logging level for successful entry and exit messages.

        Returns:
            Callable: Decorated function, or a decorator when ``func`` is absent.

        Notes:
            Representations are truncated to reduce accidental credential
            exposure. Sensitive APIs should still avoid ``log_args=True``.
        """

        def decorator(function: Callable[P, R]) -> Callable[P, R]:
            """Wrap one callable with tracing behavior.

            Args:
                function (Callable[P, R]): Callable to decorate.

            Returns:
                Callable[P, R]: Metadata-preserving traced callable.
            """

            @functools.wraps(function)
            def wrapper(*args: P.args, **kwargs: P.kwargs) -> R:
                """Log one invocation and preserve its result or exception.

                Args:
                    *args (P.args): Original positional arguments.
                    **kwargs (P.kwargs): Original keyword arguments.

                Returns:
                    R: Unmodified result from the wrapped callable.

                Raises:
                    Exception: Any wrapped exception is logged and re-raised unchanged.
                """
                name = function.__qualname__
                details = ""
                if log_args:
                    details = f" args={_bounded(args[1:])} kwargs={_bounded(kwargs)}"
                self.logger.log(level, "ENTER %s%s", name, details)
                try:
                    result = function(*args, **kwargs)
                except Exception:
                    self.logger.exception("EXCEPTION in %s", name)
                    raise
                suffix = f" result={_bounded(result)}" if log_result else ""
                self.logger.log(level, "EXIT %s%s", name, suffix)
                return result

            return wrapper

        return decorator(func) if func is not None else decorator


def _default_log_directory() -> Path:
    """Return the platform-appropriate user log directory.

    Returns:
        pathlib.Path: Windows roaming application data or ``~/.v2root/logs``.
    """
    if platform.system() == "Windows":
        return Path(os.environ.get("APPDATA", Path.home())) / "V2Root/logs"
    return Path.home() / ".v2root/logs"


def _bounded(value, limit: int = 200) -> str:
    """Return a length-limited representation suitable for diagnostic logs.

    Args:
        value (object): Value to represent.
        limit (int): Maximum number of characters before truncation.

    Returns:
        str: Original representation or a suffixed truncated representation.
    """
    rendered = repr(value)
    return rendered if len(rendered) <= limit else rendered[:limit] + "..."


logger = V2RootLogger()
debug = logger.debug
info = logger.info
warning = logger.warning
error = logger.error
critical = logger.critical
exception = logger.exception
get_logger = logger.get_logger
set_level = logger.set_level
log_function_call = logger.log_function_call


def configure_logger(**kwargs) -> V2RootLogger:
    """Configure the process-wide V2Root logger.

    Args:
        **kwargs: Keyword arguments accepted by :meth:`V2RootLogger.configure`.

    Returns:
        V2RootLogger: Configured shared logger wrapper.

    Raises:
        TypeError: If an unsupported configuration keyword is supplied.
        ValueError: If rotation settings are invalid.
        OSError: If requested file logging cannot be initialized.
    """
    return logger.configure(**kwargs)
