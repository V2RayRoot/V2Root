#ifndef LIBV2ROOT_CORE_H
#define LIBV2ROOT_CORE_H

#include "libv2root_common.h"

/*
 * Loads the V2Ray configuration from config.json.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 if config.json is not found.
 *
 * Errors:
 *   Logs an error if the configuration file does not exist.
 */
EXPORT int load_v2ray_config();

/*
 * Pings a server to measure latency.
 *
 * Establishes a TCP connection to the specified address and port to measure response time.
 *
 * Parameters:
 *   address (const char*): The server address (IP or hostname) to ping.
 *   port (int): The port to connect to.
 *
 * Returns:
 *   int: Latency in milliseconds on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for socket creation, address resolution, or connection failures.
 */
EXPORT int ping_server(const char* address, int port);

/*
 * Default port for HTTP proxy.
 */
#define DEFAULT_HTTP_PORT 1080

/*
 * Default port for SOCKS proxy.
 */
#define DEFAULT_SOCKS_PORT 1081

#endif