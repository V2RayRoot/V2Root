#ifndef V2ROOT_CORE_H
#define V2ROOT_CORE_H
/*
 * Header guard to prevent multiple inclusions of this file in a single compilation unit.
 * Ensures that the declarations are processed only once by the compiler.
 */

int load_v2ray_config(void);
 /*
  * Declares a function to load and validate the fixed v2ray configuration file "config.json".
  *
  * Parameters:
  * - None
  *
  * Returns:
  * - int: 0 on success (file "config.json" exists and is accessible), -1 on failure (file doesn’t exist or is inaccessible).
  *
  * Purpose:
  * - Validates the existence of the fixed v2ray configuration file "config.json" before use.
  *
  * Notes:
  * - The configuration file is statically set to "config.json" in the current working directory and cannot be customized.
  */

int start_v2ray();
/*
 * Declares a function to start the v2ray process using the loaded configuration.
 *
 * Parameters:
 * - None
 *
 * Returns:
 * - int: Positive integer (PID) on success, -1 on failure (e.g., v2ray.exe not found), -2 if v2ray is already running.
 *
 * Purpose:
 * - Launches the v2ray executable as a detached process, manages its PID in the registry, and enables the system proxy.
 */

int stop_v2ray();
/*
 * Declares a function to stop the currently running v2ray process.
 *
 * Parameters:
 * - None
 *
 * Returns:
 * - int: 0 on success (process terminated or no active process), -1 on failure (no valid PID or termination failed).
 *
 * Purpose:
 * - Terminates the v2ray process, clears its PID from the registry, and disables the system proxy.
 */

int ping_server(const char* address, int port);
/*
 * Declares a function to measure latency to a server via TCP connection.
 *
 * Parameters:
 * - address (const char*): A null-terminated string with the server IP address (e.g., "1.1.1.1").
 * - port (int): The port to connect to (e.g., 443).
 *
 * Returns:
 * - int: Latency in milliseconds on success, -1 on failure (e.g., connection error).
 *
 * Purpose:
 * - Tests network latency to a specified server IP and port without DNS resolution.
 */

 int parse_vless_string(const char* vless_str);
 /*
  * Declares a function to parse a VLESS string and generate a v2ray configuration file named "config.json".
  *
  * Parameters:
  * - vless_str (const char*): A null-terminated VLESS string (e.g., "vless://uuid@address:port?params").
  *
  * Returns:
  * - int: 0 on success (config file "config.json" written and proxy enabled), -1 on failure (invalid VLESS string, null parameter, port out of range, or file write error).
  *
  * Purpose:
  * - Converts a VLESS URL into a valid v2ray JSON configuration file named "config.json", supporting all mandatory components (uuid, address, port) and optional settings (encryption, flow, network type, security, etc.).
  * - Automatically enables the system proxy with fixed ports (HTTP 2300, SOCKS 2301) upon successful file creation.
  *
  * Notes:
  * - The output file is always written as "config.json" in the current working directory and cannot be customized by the user.
  * - Overwrites any existing "config.json" file without warning.
  */

 int check_server_status(const char* vless_str);
 /*
  * Declares a function to check the connection status of a VLESS server and return the latency.
  *
  * Parameters:
  * - vless_str (const char*): A null-terminated VLESS string (e.g., "vless://uuid@address:port?params").
  *
  * Returns:
  * - Positive integer: Latency in milliseconds if the connection is successful.
  * - Negative integer: Error code if the check fails:
  *   - -1: Invalid input (null parameter, invalid VLESS format, or port out of range).
  *   - -2: Winsock initialization failed.
  *   - -3: Socket creation failed.
  *   - -4: DNS resolution failed.
  *   - -5: Connection timeout.
  *
  * Purpose:
  * - Establishes a TCP connection to the server specified in the VLESS string and measures the latency in milliseconds.
  * - Provides a simple way to verify server availability and performance without modifying system settings.
  */

 void ResetNetworkProxy();
/*
 * Declares a function to fully reset all proxy settings in Windows.
 *
 * Parameters:
 * - None
 *
 * Returns:
 * - None (void)
 *
 * Purpose:
 * - Disables the system proxy, clears manual proxy settings, and resets automatic proxy configurations.
 */
#endif
/*
 * Closes the header guard, marking the end of the conditional inclusion block.
 */