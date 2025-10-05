.. _logger:

Logging System
==============

V2ROOT 1.2.0 includes a professional logging system with colored console output, file rotation, and function call tracking.

Overview
--------

The logging system provides:

- **Colored console output** (when colorama is available)
- **File logging with rotation** (5MB files, 3 backups)
- **Multiple log levels** (DEBUG, INFO, WARNING, ERROR, CRITICAL)
- **Function call tracking** with decorators
- **Thread-safe** operations
- **Contextual information** (timestamp, level, module, function, line number)

Basic Usage
-----------

Using the Logger
~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import logger

   # Log messages at different levels
   logger.debug("Debug message")
   logger.info("Info message")
   logger.warning("Warning message")
   logger.error("Error message")
   logger.critical("Critical message")

   # Log exceptions with traceback
   try:
       result = 1 / 0
   except Exception as e:
       logger.exception("Division by zero occurred")

Configuring the Logger
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import configure_logger
   import logging

   # Change log level
   configure_logger(log_level=logging.DEBUG)

   # Disable console output
   configure_logger(log_to_console=False)

   # Disable file output
   configure_logger(log_to_file=False)

   # Change log directory
   configure_logger(log_dir="/custom/log/directory")

   # Adjust file size and backup count
   configure_logger(max_file_size=10*1024*1024, backup_count=5)

Function Call Tracking
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import log_function_call

   # Decorate functions to log entry/exit
   @log_function_call
   def my_function(arg1, arg2):
       return arg1 + arg2

   # Log with arguments
   @log_function_call(log_args=True)
   def process_data(data):
       return len(data)

   # Log with result
   @log_function_call(log_result=True)
   def calculate(x, y):
       return x * y

   # Log at specific level
   @log_function_call(level=logging.WARNING)
   def critical_operation():
       pass

Advanced Usage
--------------

Custom Logger Instances
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import get_logger

   # Get a named logger
   my_logger = get_logger('my_module')

   # Use it like the main logger
   my_logger.info("Message from my module")
   my_logger.debug("Debug info")

Log Levels
~~~~~~~~~~

Available log levels (in order of severity):

- **DEBUG** (10): Detailed information, typically for diagnosing problems
- **INFO** (20): Confirmation that things are working as expected
- **WARNING** (30): Indication that something unexpected happened
- **ERROR** (40): A serious problem, software unable to perform function
- **CRITICAL** (50): A very serious error, program may be unable to continue

.. code-block:: python

   import logging
   from v2root import logger, set_level

   # Set to DEBUG to see all messages
   set_level(logging.DEBUG)

   # Set to WARNING to only see warnings and above
   set_level(logging.WARNING)

   # Set to ERROR to only see errors and critical messages
   set_level(logging.ERROR)

Log File Location
~~~~~~~~~~~~~~~~~

By default, log files are stored in:

- **Windows**: ``%APPDATA%\V2Root\logs\v2root.log``
- **Linux**: ``~/.v2root/logs/v2root.log``

You can customize this location:

.. code-block:: python

   from v2root import configure_logger

   # Custom log directory
   configure_logger(log_dir="/var/log/v2root")

Log Rotation
~~~~~~~~~~~~

Logs are automatically rotated when they reach the maximum size:

.. code-block:: python

   from v2root import configure_logger

   # Rotate at 10MB, keep 5 backup files
   configure_logger(max_file_size=10*1024*1024, backup_count=5)

   # Backup files are named: v2root.log.1, v2root.log.2, etc.

API Reference
-------------

Logger Functions
~~~~~~~~~~~~~~~~

.. autofunction:: v2root.logger.debug
   :no-index:

.. autofunction:: v2root.logger.info
   :no-index:

.. autofunction:: v2root.logger.warning
   :no-index:

.. autofunction:: v2root.logger.error
   :no-index:

.. autofunction:: v2root.logger.critical
   :no-index:

.. autofunction:: v2root.logger.exception
   :no-index:

Configuration Functions
~~~~~~~~~~~~~~~~~~~~~~~

.. autofunction:: v2root.configure_logger
   :no-index:

.. autofunction:: v2root.get_logger
   :no-index:

.. autofunction:: v2root.set_level
   :no-index:

Decorators
~~~~~~~~~~

.. autofunction:: v2root.log_function_call
   :no-index:
