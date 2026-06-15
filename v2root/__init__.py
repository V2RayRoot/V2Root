# v2root/__init__.py
"""Expose the stable public API for the V2Root Python SDK 2.0.

The package provides managed V2Root Core installation, a direct native ABI
binding, high-level runtime operations, typed value objects, subscription
services, structured errors, and explicit logging configuration. Importing the
package does not download a core, start a runtime, or create log files.
"""

from .client import V2RootClient
from .core import CoreManager
from .errors import (
    ConfigurationError,
    CoreCompatibilityError,
    CoreDownloadError,
    CoreError,
    CoreIntegrityError,
    CoreNotInstalledError,
    NativeCallError,
    V2RootError,
)
from .models import (
    InstalledCore,
    LatencyResult,
    RealtimeSpeed,
    RuntimeStatus,
    Traffic,
    UpdateResult,
    VersionInfo,
)
from .v2root import V2ROOT
from .logger import (
    configure_logger, 
    get_logger, 
    debug, 
    info, 
    warning, 
    error, 
    critical, 
    logger, 
    log_function_call,
    set_level
)
from .subscriptions import (
    SubscriptionManager,
    Subscription,
    ConfigMetadata,
    SubscriptionError,
    FetchError,
    ParseError
)
from .errors import ErrorExplanation, ErrorExplainer, PersistenceError, explain_error

__all__ = [
    'V2ROOT',
    'V2RootClient',
    'CoreManager',
    'V2RootError',
    'CoreError',
    'CoreNotInstalledError',
    'CoreDownloadError',
    'CoreIntegrityError',
    'CoreCompatibilityError',
    'NativeCallError',
    'ConfigurationError',
    'InstalledCore',
    'UpdateResult',
    'VersionInfo',
    'RuntimeStatus',
    'LatencyResult',
    'Traffic',
    'RealtimeSpeed',
    'SubscriptionManager',
    'Subscription',
    'ConfigMetadata',
    
    'SubscriptionError',
    'FetchError',
    'ParseError',
    'PersistenceError',
    'ErrorExplanation',
    'ErrorExplainer',
    'explain_error',
    
    'logger',
    'configure_logger',
    'get_logger',
    'set_level',
    'log_function_call',
    'debug',
    'info',
    'warning',
    'error',
    'critical',
    
    '__version__',
    '__author__',
    '__email__',
]

__version__ = '2.0.0'
__author__ = 'Project V2Root, Sepehr0Day'
__license__ = 'MIT'
__email__ = 'sphrz2324@gmail.com'
__url__ = 'https://github.com/V2RayRoot/V2Root'
__description__ = 'A managed Python SDK for V2Root Core, configurations, and subscriptions'
