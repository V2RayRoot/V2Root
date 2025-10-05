.. _subscription:

Subscription Management
=======================

V2ROOT 1.2.0 introduces comprehensive subscription management capabilities, allowing you to manage multiple V2Ray subscriptions with automatic updates, filtering, and metadata tracking.

Overview
--------

The subscription management system consists of three main classes:

- **SubscriptionManager**: Manages multiple subscriptions and provides global filtering
- **Subscription**: Represents a single subscription with auto-update capabilities
- **ConfigMetadata**: Stores metadata for individual configurations (latency, success rate, tags)

Basic Usage
-----------

Creating a Subscription Manager
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import SubscriptionManager

   # Create a subscription manager (storage in default location)
   manager = SubscriptionManager()

   # Or specify custom storage directory
   manager = SubscriptionManager(storage_dir="/path/to/subscriptions")

Adding Subscriptions
~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # Add a subscription
   sub = manager.add_subscription(
       url="https://example.com/subscription",
       name="My VPN",
       auto_update=True,
       update_interval=86400,  # 24 hours
       fetch_now=True
   )

   print(f"Added subscription with {len(sub.configs)} configurations")

Updating Subscriptions
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # Update a specific subscription
   configs = manager.update_subscription(sub.id)

   # Update all subscriptions
   results = manager.update_all()

   # Check update results
   for sub_id, result in results.items():
       if isinstance(result, list):
           print(f"Updated {sub_id}: {len(result)} configs")
       else:
           print(f"Failed {sub_id}: {result}")

Filtering Configurations
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # Filter by protocol
   vless_configs = manager.filter_configs(protocols=['vless'])

   # Filter by latency (requires testing first)
   fast_configs = manager.filter_configs(max_latency=200)

   # Filter by success rate (requires testing first)
   reliable_configs = manager.filter_configs(min_success_rate=0.8)

   # Filter by subscription tags
   tagged_configs = manager.filter_configs(subscription_tags=['premium'])

   # Filter by config tags
   tagged_configs = manager.filter_configs(config_tags=['fast', 'stable'])

   # Filter by name pattern (regex, case-insensitive)
   us_configs = manager.filter_configs(name_contains=r'(US|United States)')

   # Combine multiple filters
   best_configs = manager.filter_configs(
       protocols=['vless', 'vmess'],
       max_latency=150,
       min_success_rate=0.9,
       name_contains='Japan'
   )

Testing Configurations
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import V2ROOT

   v2 = V2ROOT()

   # Test configurations and update metadata
   all_configs = manager.get_all_configs()
   for config in all_configs:
       try:
           latency = v2.test_connection(config.config_string)
           config.update_test_result(latency, success=True)
           print(f"{config.name}: {latency}ms")
       except Exception as e:
           config.update_test_result(-1, success=False)
           print(f"{config.name}: Failed - {e}")

   # Now you can filter by latency and success rate
   fast_configs = manager.filter_configs(max_latency=200)

Sorting Configurations
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # Get a subscription
   sub = manager.get_subscription(sub_id)

   # Sort by latency (fastest first)
   sorted_by_latency = sub.sort_configs(by='latency')

   # Sort by success rate (most reliable first)
   sorted_by_success = sub.sort_configs(by='success_rate', reverse=True)

   # Sort by name
   sorted_by_name = sub.sort_configs(by='name')

   # Sort by protocol
   sorted_by_protocol = sub.sort_configs(by='protocol')

Working with Tags
~~~~~~~~~~~~~~~~~

.. code-block:: python

   # Add subscription with tags
   sub = manager.add_subscription(
       url="https://example.com/subscription",
       name="Premium VPN",
       tags=['premium', 'fast', 'reliable']
   )

   # Add tags to configurations
   for config in sub.configs:
       if 'Japan' in config.name:
           config.tags.append('japan')
       if config.last_latency > 0 and config.last_latency < 100:
           config.tags.append('fast')

   # Filter by tags
   premium_configs = manager.filter_configs(subscription_tags=['premium'])
   japan_configs = manager.filter_configs(config_tags=['japan'])

Auto-Update
~~~~~~~~~~~

.. code-block:: python

   # Enable auto-update when adding subscription
   sub = manager.add_subscription(
       url="https://example.com/subscription",
       auto_update=True,
       update_interval=3600  # Update every hour
   )

   # Or enable later
   sub.auto_update = True
   sub.start_auto_update()

   # Stop auto-update
   sub.stop_auto_update()

Statistics
~~~~~~~~~~

.. code-block:: python

   # Get subscription statistics
   stats = sub.get_statistics()
   print(f"Total configs: {stats['total_configs']}")
   print(f"Tested configs: {stats['tested_configs']}")
   print(f"Average latency: {stats['average_latency']}ms")
   print(f"Update success rate: {stats['success_rate']:.2%}")
   print(f"Protocols: {stats['protocols']}")

   # Get config metadata
   for config in sub.configs:
       print(f"{config.name}:")
       print(f"  Protocol: {config.protocol}")
       print(f"  Address: {config.address}:{config.port}")
       print(f"  Latency: {config.last_latency}ms")
       print(f"  Success rate: {config.get_success_rate():.2%}")
       print(f"  Tags: {', '.join(config.tags)}")

Getting Configuration Strings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The simplest way to get configuration strings from subscriptions:

.. code-block:: python

   from v2root import SubscriptionManager

   manager = SubscriptionManager()

   # Get all config strings from all subscriptions
   all_configs = manager.get_all_configs()
   print(f"Total: {len(all_configs)} configurations")
   
   # Print first few configs
   for config in all_configs[:5]:
       print(config[:50] + "...")  # First 50 characters

   # Get configs from a specific subscription
   sub_id = "70e98b603eb9884b728381f126452104"
   configs = manager.get_configs_from_subscription(sub_id)
   
   if configs:
       print(f"Found {len(configs)} configs in subscription")
       for config in configs:
           print(config)
   else:
       print("Subscription not found")

   # Get configs from a subscription object directly
   sub = manager.get_subscription(sub_id)
   if sub:
       configs = sub.get_configs()
       print(f"Subscription has {len(configs)} configs")

Using Retrieved Configurations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once you have the configuration strings, you can use them with V2ROOT:

.. code-block:: python

   from v2root import V2ROOT, SubscriptionManager

   # Get all configs
   manager = SubscriptionManager()
   all_configs = manager.get_all_configs()

   # Test and use the first config
   if all_configs:
       v2 = V2ROOT()
       
       # Test the config
       try:
           latency = v2.test_connection(all_configs[0])
           print(f"Config tested: {latency}ms")
           
           # Use it
           v2.set_config_string(all_configs[0])
           v2.start()
           
           # ... do your work ...
           
           v2.stop()
       except Exception as e:
           print(f"Failed: {e}")

Testing Multiple Configurations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Batch test configurations from subscriptions:

.. code-block:: python

   from v2root import V2ROOT, SubscriptionManager

   manager = SubscriptionManager()
   v2 = V2ROOT()

   # Get all configs
   all_configs = manager.get_all_configs()
   
   print(f"Testing {len(all_configs)} configurations...")
   
   tested_configs = []
   for i, config in enumerate(all_configs, 1):
       try:
           print(f"[{i}/{len(all_configs)}] Testing...", end=" ")
           latency = v2.test_connection(config)
           tested_configs.append((config, latency))
           print(f"✓ {latency}ms")
       except Exception as e:
           print(f"✗ Failed: {e}")
   
   # Sort by latency
   tested_configs.sort(key=lambda x: x[1])
   
   # Use the fastest one
   if tested_configs:
       best_config, best_latency = tested_configs[0]
       print(f"\nBest config: {best_latency}ms")
       v2.set_config_string(best_config)
       v2.start()

Advanced Usage
--------------

Custom Storage Location
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   import os

   # Use project-specific directory
   storage_dir = os.path.join(os.getcwd(), 'my_subscriptions')
   manager = SubscriptionManager(storage_dir=storage_dir)

Batch Operations
~~~~~~~~~~~~~~~~

.. code-block:: python

   # Add multiple subscriptions
   urls = [
       "https://provider1.com/subscription",
       "https://provider2.com/subscription",
       "https://provider3.com/subscription"
   ]

   for i, url in enumerate(urls):
       manager.add_subscription(
           url=url,
           name=f"Provider {i+1}",
           priority=i,
           fetch_now=True
       )

   # Update all and collect results
   results = manager.update_all()
   successful = sum(1 for r in results.values() if isinstance(r, list))
   print(f"Updated {successful}/{len(results)} subscriptions")

Finding Best Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import V2ROOT

   v2 = V2ROOT()

   # Test all configurations
   all_configs = manager.get_all_configs()
   tested_configs = []

   for config in all_configs:
       try:
           latency = v2.test_connection(config.config_string)
           config.update_test_result(latency, success=True)
           tested_configs.append(config)
       except:
           config.update_test_result(-1, success=False)

   # Find best configuration
   if tested_configs:
       best_config = min(tested_configs, key=lambda c: c.last_latency)
       print(f"Best config: {best_config.name} ({best_config.last_latency}ms)")

       # Use it
       v2.set_config_string(best_config.config_string)
       v2.start()

Exception Handling
~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from v2root import SubscriptionError, FetchError, ParseError

   try:
       sub = manager.add_subscription(
           url="https://example.com/subscription",
           fetch_now=True
       )
   except FetchError as e:
       print(f"Failed to fetch subscription: {e}")
   except ParseError as e:
       print(f"Failed to parse subscription: {e}")
   except SubscriptionError as e:
       print(f"Subscription error: {e}")

API Reference
-------------

SubscriptionManager
~~~~~~~~~~~~~~~~~~~

.. autoclass:: v2root.SubscriptionManager
   :members:
   :undoc-members:
   :show-inheritance:
   :no-index:

**Key Methods:**

- ``get_all_configs(enabled_only=True)`` - Get all configuration strings from all subscriptions
- ``get_configs_from_subscription(subscription_id)`` - Get all configuration strings from a specific subscription

Subscription
~~~~~~~~~~~~

.. autoclass:: v2root.Subscription
   :members:
   :undoc-members:
   :show-inheritance:
   :no-index:

**Key Methods:**

- ``get_configs()`` - Get all configuration strings from this subscription
- ``fetch(timeout=30)`` - Fetch and parse the subscription
- ``start_auto_update()`` - Start automatic updates
- ``stop_auto_update()`` - Stop automatic updates

ConfigMetadata
~~~~~~~~~~~~~~

.. autoclass:: v2root.ConfigMetadata
   :members:
   :undoc-members:
   :show-inheritance:
   :no-index:

Exceptions
~~~~~~~~~~

.. autoclass:: v2root.SubscriptionError
   :members:
   :show-inheritance:
   :no-index:

.. autoclass:: v2root.FetchError
   :members:
   :show-inheritance:
   :no-index:

.. autoclass:: v2root.ParseError
   :members:
   :show-inheritance:
   :no-index:
