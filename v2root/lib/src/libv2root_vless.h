#ifndef LIBV2ROOT_VLESS_H
#define LIBV2ROOT_VLESS_H

#include <stdio.h>
#include "libv2root_common.h"

/*
 * Parses a VLESS configuration string and writes the resulting JSON configuration to a file.
 *
 * Supports the VLESS protocol with comprehensive transport and security options, including:
 * - Transport protocols: TCP, HTTP/2 (h2), WebSocket (ws), mKCP, QUIC, gRPC
 * - Security options: None, TLS, Reality
 * - Encryption: None (default)
 * - Flow control: Optional flow parameter for advanced routing
 * - TCP settings: Custom header types (e.g., none, http)
 * - HTTP/2 settings: Path, host, custom headers
 * - WebSocket settings: Path, host header
 * - mKCP settings: Header type, seed, congestion control (e.g., BBR)
 * - QUIC settings: Security, key, header type
 * - gRPC settings: Service name, multi-mode support
 * - TLS settings: Server Name Indication (SNI), ALPN, fingerprint
 * - Reality settings: Public key, short IDs, spiderX, fingerprint
 * - Inbound proxies: HTTP and SOCKS with configurable ports
 *
 * Parameters:
 *   vless_str (const char*): The VLESS configuration string (e.g., vless://uuid@address:port?params).
 *   fp (FILE*): File pointer to write the JSON configuration.
 *   http_port (int): The HTTP proxy port (defaults to DEFAULT_HTTP_PORT if invalid or <= 0).
 *   socks_port (int): The SOCKS proxy port (defaults to DEFAULT_SOCKS_PORT if invalid or <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for invalid input, incorrect VLESS prefix, parsing failures, invalid port/address,
 *   or parameter buffer overflow.
 */
EXPORT int parse_vless_string(const char* vless_str, FILE* fp, int http_port, int socks_port);

#endif