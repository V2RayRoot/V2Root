Logging
-------

Python SDK logging and native core logging are configured independently.

Python Logging
--------------

Importing V2Root performs no file I/O and installs only a ``NullHandler``.
Configure output explicitly:

.. code-block:: python

   import logging
   from v2root import configure_logger

   configure_logger(
       log_level=logging.INFO,
       log_to_console=True,
       log_to_file=True,
       log_dir="./logs",
       max_file_size=5 * 1024 * 1024,
       backup_count=3,
   )

Native Logging
--------------

Configure V2Root Core before startup:

.. code-block:: python

   client.configure_logging(
       level="warning",
       output="./logs/v2root-core.log",
   )

Security
--------

Do not log full share URIs or complete configuration JSON. The function-call
decorator truncates representations, but sensitive methods should still avoid
``log_args=True``.
