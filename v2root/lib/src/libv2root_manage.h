#ifndef LIBV2ROOT_MANAGE_H
#define LIBV2ROOT_MANAGE_H

#include "libv2root_common.h"

/*
 * Initializes the V2Ray core with a configuration file and executable path.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   v2ray_path (const char*): Path to the V2Ray executable (e.g., v2ray.exe on Windows).
 *
 * Returns:
 *   int: 0 on success, -1 on failure (e.g., invalid paths or file not found).
 *
 * Errors:
 *   Logs errors if the config file or executable is not found or paths are too long.
 */
EXPORT int init_v2ray(const char* config_file, const char* v2ray_path);

/*
 * Resets the network proxy settings.
 *
 * Disables system proxy and stops any running V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if proxy reset or process termination fails.
 */
EXPORT int reset_network_proxy();

/*
 * Parses a V2Ray configuration string and generates a config file.
 *
 * Supports VLESS, VMess, and Shadowsocks protocols.
 *
 * Parameters:
 *   config_string (const char*): V2Ray configuration string (e.g., vless://, vmess://, ss://).
 *   http_port (int): HTTP proxy port.
 *   socks_port (int): SOCKS proxy port.
 *
 * Returns:
 *   int: 0 on success, -1 on failure (e.g., invalid string or file write error).
 *
 * Errors:
 *   Logs errors for invalid protocols or file operations.
 */
EXPORT int parse_config_string(const char* config_string, int http_port, int socks_port);

/*
 * Starts the V2Ray process.
 *
 * Configures system proxy and launches V2Ray with the specified ports.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port.
 *   socks_port (int): SOCKS proxy port.
 *
 * Returns:
 *   int: Process ID (PID) on success, negative error code on failure.
 *
 * Errors:
 *   Logs errors if V2Ray initialization or process start fails.
 */
EXPORT int start_v2ray(int http_port, int socks_port);

/*
 * Starts the V2Ray process with specified ports and stores the process ID.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port.
 *   socks_port (int): SOCKS proxy port.
 *   pid (PID_TYPE*): Pointer to store the process ID.
 *
 * Returns:
 *   int: 0 on success, negative error code on failure.
 *
 * Errors:
 *   Logs errors for initialization, proxy setup, or process start failures.
 */
EXPORT int start_v2ray_with_pid(int http_port, int socks_port, PID_TYPE* pid);

/*
 * Stops the V2Ray process.
 *
 * Terminates the running V2Ray process and resets system proxy settings.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if process termination or proxy reset fails.
 */
EXPORT int stop_v2ray();

/*
 * Tests a V2Ray configuration for connectivity and latency.
 *
 * Starts a temporary V2Ray process to test the configuration and measures latency.
 *
 * Parameters:
 *   config_string (const char*): V2Ray configuration string to test.
 *   latency (int*): Pointer to store the measured latency in milliseconds.
 *   http_port (int): HTTP proxy port.
 *   socks_port (int): SOCKS proxy port.
 *
 * Returns:
 *   int: 0 on success, negative error code on failure (e.g., -2 for process start failure).
 *
 * Errors:
 *   Logs errors for parsing, process start, or connection test failures.
 */
EXPORT int test_config_connection(const char* config_string, int* latency, int http_port, int socks_port);

#endif