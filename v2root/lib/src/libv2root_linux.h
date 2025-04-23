#ifndef LIBV2ROOT_LINUX_H
#define LIBV2ROOT_LINUX_H

#include "libv2root_common.h"
#include <sys/types.h>

/*
 * Starts a V2Ray process on Linux using the specified configuration file.
 *
 * Creates and starts a V2Ray service, storing the process ID.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   pid (pid_t*): Pointer to store the process ID of the started V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if the config file is missing, a service is already running, or service creation/start fails.
 */
EXPORT int linux_start_v2ray_process(const char* config_file, pid_t* pid);

/*
 * Stops a V2Ray process on Linux.
 *
 * Stops and removes the V2Ray service, resetting network proxy settings.
 *
 * Parameters:
 *   pid (pid_t): The process ID of the V2Ray process to stop.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if stopping or removing the service, or resetting the proxy fails.
 */
EXPORT int linux_stop_v2ray_process(pid_t pid);

/*
 * Tests the connectivity and latency of a V2Ray configuration on Linux.
 *
 * Sends an HTTP request through the specified proxy port to measure latency.
 *
 * Parameters:
 *   http_port (int): The HTTP proxy port to test.
 *   socks_port (int): The SOCKS proxy port (unused).
 *   latency (int*): Pointer to store the measured latency in milliseconds.
 *   pid (pid_t): The process ID of the V2Ray process (unused).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if curl initialization or the HTTP request fails.
 */
EXPORT int linux_test_connection(int http_port, int socks_port, int* latency, pid_t pid);

/*
 * Enables system proxy settings on Linux for HTTP and SOCKS protocols.
 *
 * Sets environment variables for proxies in the parent shell.
 *
 * Parameters:
 *   http_port (int): The port for the HTTP proxy.
 *   socks_port (int): The port for the SOCKS proxy.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if setting proxy variables or executing the set command fails.
 */
EXPORT int linux_enable_system_proxy(int http_port, int socks_port);

/*
 * Disables system proxy settings on Linux.
 *
 * Unsets proxy environment variables in the parent shell.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if unsetting proxy variables or executing the unset command fails.
 */
EXPORT int linux_disable_system_proxy(void);

/*
 * Resets network proxy settings on Linux.
 *
 * Unsets all proxy environment variables in the parent shell.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if unsetting proxy variables or executing the unset command fails.
 */
EXPORT int linux_reset_network_proxy(void);

#endif