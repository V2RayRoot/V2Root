#ifndef LIBV2ROOT_WIN_H
#define LIBV2ROOT_WIN_H

#include "libv2root_common.h"

#ifdef _WIN32
    #include <windows.h>

/*
 * Saves the V2Ray process ID to the Windows registry.
 *
 * Stores the PID under HKEY_CURRENT_USER\Software\V2Root.
 *
 * Parameters:
 *   pid (PID_TYPE): Process ID to save.
 */
EXPORT void save_pid_to_registry(PID_TYPE pid);

/*
 * Loads the V2Ray process ID from the Windows registry.
 *
 * Retrieves the PID from HKEY_CURRENT_USER\Software\V2Root.
 *
 * Returns:
 *   PID_TYPE: Stored PID, or 0 if not found.
 */
EXPORT PID_TYPE load_pid_from_registry();

/*
 * Removes the V2Ray process ID from the Windows registry.
 *
 * Deletes the PID entry from HKEY_CURRENT_USER\Software\V2Root.
 */
EXPORT void remove_pid_from_registry();

/*
 * Resets the system network proxy settings on Windows.
 *
 * Clears proxy settings and stops any running V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int win_reset_network_proxy();

/*
 * Enables system proxy settings on Windows for HTTP and SOCKS protocols.
 *
 * Configures registry and WinHTTP for the specified ports.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port.
 *   socks_port (int): SOCKS proxy port.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int win_enable_system_proxy(int http_port, int socks_port);

/*
 * Disables system proxy settings on Windows.
 *
 * Clears proxy settings from registry and WinHTTP.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int win_disable_system_proxy();

/*
 * Starts a V2Ray process on Windows using the specified configuration file and executable path.
 *
 * Launches v2ray.exe and stores the process ID.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   v2ray_path (const char*): Path to the V2Ray executable.
 *   pid (PID_TYPE*): Pointer to store the process ID.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int win_start_v2ray_process(const char* config_file, const char* v2ray_path, PID_TYPE* pid);

/*
 * Stops a V2Ray process on Windows using the specified process ID.
 *
 * Terminates the process and removes its PID from the registry.
 *
 * Parameters:
 *   pid (PID_TYPE): Process ID to stop.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int win_stop_v2ray_process(PID_TYPE pid);

/*
 * Tests the connectivity and latency of a V2Ray configuration on Windows.
 *
 * Sends an HTTP request through the proxy to measure latency.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port.
 *   latency (int*): Pointer to store the latency in milliseconds.
 *   hProcess (HANDLE): Handle to the V2Ray process.
 *
 * Returns:
 *   int: 0 on success, negative error code on failure.
 */
EXPORT int win_test_connection(int http_port, int* latency, HANDLE hProcess);

#endif

#endif