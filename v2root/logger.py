import os
import sys
import logging
import platform
import traceback
import functools
from logging.handlers import RotatingFileHandler
from datetime import datetime
from typing import Optional, Callable, Any

# Try to import colorama for colored console output
try:
    from colorama import init, Fore, Style
    init(autoreset=True)
    HAS_COLORAMA = True
except ImportError:
    HAS_COLORAMA = False

class V2RootLogger:
    """
    Professional logging system for V2Root.
    
    Features:
    - Console output with color-coding
    - File output with rotation
    - Multiple log levels (DEBUG, INFO, WARNING, ERROR, CRITICAL)
    - Contextual information (timestamp, level, module, function, line number)
    - Thread safety
    - Performance optimized
    """
    
    # Singleton instance
    _instance = None
    
    # Log levels
    DEBUG = logging.DEBUG
    INFO = logging.INFO
    WARNING = logging.WARNING
    ERROR = logging.ERROR
    CRITICAL = logging.CRITICAL
    
    def __new__(cls, *args, **kwargs):
        """Ensure only one logger instance exists (Singleton pattern)."""
        if cls._instance is None:
            cls._instance = super(V2RootLogger, cls).__new__(cls)
            cls._instance._initialized = False
        return cls._instance
    
    def __init__(self, log_level=logging.INFO, log_to_file=True, log_to_console=True,
                 log_dir=None, max_file_size=5*1024*1024, backup_count=3):
        """
        Initialize the logging system.
        
        Args:
            log_level: Minimum log level to record
            log_to_file: Whether to log to file
            log_to_console: Whether to log to console
            log_dir: Directory to store log files (default: platform-specific user data dir)
            max_file_size: Maximum size of each log file in bytes before rotation
            backup_count: Number of backup log files to keep
        """
        # Only initialize once (singleton pattern)
        if getattr(self, '_initialized', False):
            return
            
        self.log_level = log_level
        self.log_to_file = log_to_file
        self.log_to_console = log_to_console
        
        # Determine log directory
        if log_dir is None:
            if platform.system() == "Windows":
                log_dir = os.path.join(os.environ.get('APPDATA', ''), 'V2Root', 'logs')
            else:
                log_dir = os.path.join(os.path.expanduser('~'), '.v2root', 'logs')
        
        os.makedirs(log_dir, exist_ok=True)
        self.log_file = os.path.join(log_dir, 'v2root.log')
        
        # Create the logger
        self.logger = logging.getLogger('v2root')
        self.logger.setLevel(log_level)
        self.logger.propagate = False  # Don't propagate to parent loggers
        
        # Clear existing handlers
        if self.logger.handlers:
            self.logger.handlers.clear()
        
        # Add file handler if requested
        if log_to_file:
            file_formatter = logging.Formatter(
                '%(asctime)s [%(levelname)s] %(name)s:%(funcName)s:%(lineno)d - %(message)s'
            )
            file_handler = RotatingFileHandler(
                self.log_file, 
                maxBytes=max_file_size,
                backupCount=backup_count,
                encoding='utf-8'
            )
            file_handler.setFormatter(file_formatter)
            file_handler.setLevel(log_level)
            self.logger.addHandler(file_handler)
        
        # Add console handler if requested
        if log_to_console:
            if HAS_COLORAMA:
                # Custom formatter with colors
                class ColoredFormatter(logging.Formatter):
                    COLORS = {
                        'DEBUG': Fore.CYAN,
                        'INFO': Fore.GREEN,
                        'WARNING': Fore.YELLOW,
                        'ERROR': Fore.RED,
                        'CRITICAL': Fore.RED + Style.BRIGHT
                    }
                    
                    def format(self, record):
                        levelname = record.levelname
                        if levelname in self.COLORS:
                            record.levelname = f"{self.COLORS[levelname]}{levelname}{Style.RESET_ALL}"
                        return super().format(record)
                
                console_formatter = ColoredFormatter(
                    '%(asctime)s [%(levelname)s] %(message)s',
                    datefmt='%H:%M:%S'
                )
            else:
                console_formatter = logging.Formatter(
                    '%(asctime)s [%(levelname)s] %(message)s',
                    datefmt='%H:%M:%S'
                )
                
            console_handler = logging.StreamHandler(sys.stdout)
            console_handler.setFormatter(console_formatter)
            console_handler.setLevel(log_level)
            self.logger.addHandler(console_handler)
        
        self._initialized = True
        
        # Log system info on startup
        self.logger.debug(f"V2Root logging initialized - Level: {logging.getLevelName(log_level)}")
        if log_to_file:
            self.logger.debug(f"Log file: {self.log_file}")
        self.logger.debug(f"Platform: {platform.system()} {platform.release()}")
        self.logger.debug(f"Python: {platform.python_version()}")
    
    def get_logger(self, name=None):
        """Get a named logger that inherits settings from the main logger."""
        if name:
            logger = logging.getLogger(f'v2root.{name}')
        else:
            logger = self.logger
        return logger
    
    def debug(self, msg, *args, **kwargs):
        """Log a debug message."""
        self.logger.debug(msg, *args, **kwargs)
    
    def info(self, msg, *args, **kwargs):
        """Log an info message."""
        self.logger.info(msg, *args, **kwargs)
    
    def warning(self, msg, *args, **kwargs):
        """Log a warning message."""
        self.logger.warning(msg, *args, **kwargs)
    
    def error(self, msg, *args, **kwargs):
        """Log an error message."""
        self.logger.error(msg, *args, **kwargs)
    
    def critical(self, msg, *args, **kwargs):
        """Log a critical message."""
        self.logger.critical(msg, *args, **kwargs)
    
    def exception(self, msg, *args, exc_info=True, **kwargs):
        """Log an exception with traceback."""
        self.logger.exception(msg, *args, exc_info=exc_info, **kwargs)
    
    def set_level(self, level):
        """Change the logging level."""
        self.log_level = level
        self.logger.setLevel(level)
        for handler in self.logger.handlers:
            handler.setLevel(level)
    
    def log_function_call(self, func=None, log_args=False, log_result=False, level=logging.DEBUG):
        """
        Decorator to log function entry and exit.
        
        Args:
            func: The function to decorate
            log_args: Whether to log function arguments
            log_result: Whether to log function return value
            level: Log level to use
        
        Returns:
            Decorated function
        """
        def decorator(fn):
            @functools.wraps(fn)
            def wrapper(*args, **kwargs):
                fn_name = fn.__qualname__
                # Log function entry
                if log_args:
                    # Format args and kwargs, but limit their string representation
                    args_str = [str(arg)[:100] + '...' if len(str(arg)) > 100 else str(arg) for arg in args[1:]]
                    kwargs_str = {k: str(v)[:100] + '...' if len(str(v)) > 100 else str(v) for k, v in kwargs.items()}
                    self.logger.log(level, f"ENTER {fn_name} - Args: {args_str}, Kwargs: {kwargs_str}")
                else:
                    self.logger.log(level, f"ENTER {fn_name}")
                
                # Call the function
                try:
                    result = fn(*args, **kwargs)
                    # Log function exit with result
                    if log_result:
                        result_str = str(result)[:100] + '...' if len(str(result)) > 100 else str(result)
                        self.logger.log(level, f"EXIT {fn_name} - Result: {result_str}")
                    else:
                        self.logger.log(level, f"EXIT {fn_name}")
                    return result
                except Exception as e:
                    self.logger.error(f"EXCEPTION in {fn_name}: {str(e)}")
                    self.logger.debug(f"Traceback: {traceback.format_exc()}")
                    raise
            return wrapper
        
        # Handle both @log_function_call and @log_function_call()
        if func is not None:
            return decorator(func)
        return decorator

# Create a default logger instance
logger = V2RootLogger()

# Convenience functions
debug = logger.debug
info = logger.info
warning = logger.warning
error = logger.error
critical = logger.critical
exception = logger.exception
get_logger = logger.get_logger
set_level = logger.set_level
log_function_call = logger.log_function_call

def configure_logger(log_level=None, log_to_file=None, log_to_console=None, 
                    log_dir=None, max_file_size=None, backup_count=None):
    """
    Configure the global logger settings.
    
    Only provided parameters will be changed.
    """
    global logger
    
    params = {}
    if log_level is not None:
        params['log_level'] = log_level
    if log_to_file is not None:
        params['log_to_file'] = log_to_file
    if log_to_console is not None:
        params['log_to_console'] = log_to_console
    if log_dir is not None:
        params['log_dir'] = log_dir
    if max_file_size is not None:
        params['max_file_size'] = max_file_size
    if backup_count is not None:
        params['backup_count'] = backup_count
        
    if params:
        # Re-initialize with new parameters
        logger = V2RootLogger(**params)
    
    return logger
