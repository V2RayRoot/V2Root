Subscription Management
=======================

The subscription subsystem separates transport, parsing, entities,
persistence, scheduling, and application-service coordination.

Basic Usage
-----------

.. code-block:: python

   from v2root import SubscriptionManager

   with SubscriptionManager() as manager:
       source = manager.add_subscription(
           "https://example.com/subscription",
           name="Primary",
           tags=["production"],
       )
       print(source.get_statistics())

Persistence
-----------

Each source is stored as an independent JSON document and replaced atomically.
Use a custom location:

.. code-block:: python

   manager = SubscriptionManager(storage_dir="./application-data/subscriptions")

Parsing
-------

The parser accepts plain-text URI lines and Base64-encoded documents. It
deduplicates exact URIs and extracts VMess, VLESS, Trojan, Shadowsocks, and SSR
metadata. Existing latency history and tags survive updates by exact URI match.

Filtering and Ranking
---------------------

.. code-block:: python

   vless = manager.filter_configs(
       protocols=["vless"],
       max_latency=500,
       subscription_tags=["production"],
   )

   best = manager.best_config(max_latency=500)

Only tested configurations can be selected as ``best``.

Testing Through V2Root Core
---------------------------

.. code-block:: python

   from v2root import V2RootClient

   client = V2RootClient()
   results = client.test_subscription(manager, source.id, timeout=10)

Measurements are persisted in the owning subscription.

Scheduled Updates
-----------------

.. code-block:: python

   source = manager.add_subscription(
       "https://example.com/subscription",
       auto_update=True,
       update_interval=3600,
   )

Use the manager as a context manager or call ``close()`` to join scheduler
threads.
