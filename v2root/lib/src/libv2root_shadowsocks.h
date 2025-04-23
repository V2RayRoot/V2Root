#ifndef LIBV2ROOT_SHADOWSOCKS_H
#define LIBV2ROOT_SHADOWSOCKS_H

#include <stdio.h>
#include "libv2root_common.h"

/*
 * Parses a Shadowsocks configuration string and writes the resulting JSON configuration to a file.
 *
 * Supports the Shadowsocks protocol with comprehensive configuration options, including:
 * - Encryption methods: AES-256-GCM, AES-128-GCM, Chacha20-Poly1305, and others supported by Shadowsocks
 * - Transport protocols: TCP, UDP
 * - Security options: TLS, none
 * - Plugin support: Custom plugins with plugin options (e.g., v2ray-plugin, obfs)
 * - Query parameters:
 *   - plugin: Specifies the plugin (e.g., v2ray-plugin)
 *   - plugin-opts: Plugin-specific options
 *   - tag: Custom tag for the outbound configuration
 *   - level: User level for access control
 *   - ota: One-time authentication (true/false)
 *   - network: Transport network (TCP/UDP)
 *   - security: Stream security (e.g., TLS)
 * - Inbound proxies: HTTP and SOCKS with configurable ports
 *
 * Parameters:
 *   ss_str (const char*): The Shadowsocks configuration string (e.g., ss://base64(method:password)@address:port?params).
 *   fp (FILE*): File pointer to write the JSON configuration.
 *   http_port (int): The HTTP proxy port (defaults to DEFAULT_HTTP_PORT if invalid or <= 0).
 *   socks_port (int): The SOCKS proxy port (defaults to DEFAULT_SOCKS_PORT if invalid or <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for invalid input, incorrect Shadowsocks prefix, Base64 decoding failures, invalid method:password format,
 *   invalid address/port, or memory allocation failures.
 */
EXPORT int parse_shadowsocks_string(const char* ss_str, FILE* fp, int http_port, int socks_port);

#endif