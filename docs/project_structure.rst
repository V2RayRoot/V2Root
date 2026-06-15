Project Structure
=================

The 2.x SDK separates infrastructure, domain models, native ABI concerns, and
application services. Public imports remain available from ``v2root`` while
advanced users can depend on narrower subpackages.

Client API
----------

``v2root/client.py``
  High-level configuration, lifecycle, metrics, latency testing, and
  subscription integration.

``v2root/v2root.py``
  Compatibility facade preserving the familiar ``V2ROOT`` class.

Core Management
---------------

``v2root/core/manager.py``
  Secure installation, update, activation, removal, and rollback.

``v2root/core/transport.py``
  Injectable GitHub metadata and binary download transport.

``v2root/core/platforms.py``
  Operating-system and architecture normalization.

Native Boundary
---------------

``v2root/native.py``
  Strict ABI declarations, UTF-8 conversion, native memory ownership, JSON
  decoding, and synchronized calls.

Models and Errors
-----------------

``v2root/models/``
  Separate core, runtime, and testing value objects.

``v2root/errors/``
  Typed exceptions plus structured explanations for current failures and
  legacy numeric errors.

Subscriptions
-------------

``v2root/subscriptions/entities.py``
  Subscription and configuration domain entities. This distinct name avoids
  confusion with the public ``v2root/models/`` value-object package.

``v2root/subscriptions/parser.py``
  Plain-text, Base64, VMess, URI, IPv6, and deduplication logic.

``v2root/subscriptions/transport.py``
  Bounded network fetching with explicit decoding.

``v2root/subscriptions/repository.py``
  Atomic JSON persistence.

``v2root/subscriptions/scheduler.py``
  Joinable background update lifecycle.

``v2root/subscriptions/manager.py``
  Application service for concurrent updates, filtering, ranking, test result
  recording, and persistence coordination.

Tests
-----

Tests are separated by subsystem under ``tests/``. Native and network
dependencies are injectable, allowing deterministic unit tests without
shipping a core binary or contacting external services.
