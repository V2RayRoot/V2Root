Configuration and Parser Options
================================

Supported Share Schemes
-----------------------

* VLESS
* VMess
* Trojan
* Shadowsocks

The subscription metadata parser also recognizes SSR entries.

Common Parser Options
---------------------

Pass additional native parser options to ``V2RootClient.parse``:

.. code-block:: python

   normalized = client.parse(
       "vless://REPLACE_ME",
       routingMode="proxy",
       geositeDomain="ir",
       geositeDNS="1.1.1.1",
       geositeRules=[
           {"domain": "ads", "action": "block"},
           {"domain": "ir", "action": "direct"},
       ],
       geoipRules=[
           {"country": "private", "action": "direct"},
       ],
   )

Port Defaults
-------------

* HTTP: ``2300`` in the Python SDK
* SOCKS: ``2301`` in the Python SDK

Both ports must be integers from ``1`` through ``65535`` and must differ.

Geo Options
-----------

``geosite_path`` can be supplied during client construction and is forwarded
to parser and runtime options.

The authoritative native option schema is maintained by V2Root Core.
