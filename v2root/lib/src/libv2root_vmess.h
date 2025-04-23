#ifndef LIBV2ROOT_VMESS_H
#define LIBV2ROOT_VMESS_H

#include <stdio.h>
#include "libv2root_common.h"

/*
 * Parses a VMess configuration string and writes the resulting JSON configuration to a file.
 *
 * Supports the VMess protocol with comprehensive transport and security options, including:
 * - Transport protocols: TCP, HTTP/2 (h2), WebSocket (ws), mKCP, QUIC, gRPC
 * - Security options: None, TLS
 * - Encryption methods: Auto (default), AES-128-GCM, Chacha20-Poly1305, none
 * - TCP settings: Custom header types (e.g., none, http)
 * - HTTP/2 settings: Path, host, custom headers
 * - WebSocket settings: Path, host header
 * - mKCP settings: Header type, seed, congestion control (e.g., BBR)
 * - QUIC settings: Security, key, header type
 * - gRPC settings: Service name, multi-mode support
 * - TLS settings: Server Name Indication (SNI), ALPN
 * - Inbound proxies: HTTP and SOCKS with configurable ports
 * - Additional features: AlterId for backward compatibility
 *
 * Parameters:
 *   vmess_str (const char*): The VMess configuration string (e.g., vmess://base64_encoded_json).
 *   fp (FILE*): File pointer to write the JSON configuration.
 *   http_port (int): The HTTP proxy port (defaults to DEFAULT_HTTP_PORT if invalid or <= 0).
 *   socks_port (int): The SOCKS proxy port (defaults to DEFAULT_SOCKS_PORT if invalid or <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for invalid input, incorrect VMess prefix, Base64 decoding failures, missing required fields,
 *   invalid port/address, or memory allocation failures.
 */
EXPORT int parse_vmess_string(const char* vmess_str, FILE* fp, int http_port, int socks_port);

#endif