#ifndef LIBV2ROOT_SERVICE_H
#define LIBV2ROOT_SERVICE_H

#include <sys/types.h>

/*
 * Creates a systemd user service for V2Ray.
 *
 * Generates a systemd service file with the specified configuration and proxy settings.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   http_port (int): The HTTP proxy port.
 *   socks_port (int): The SOCKS proxy port.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if service path initialization, directory creation, file writing, or systemd reload fails.
 */
int create_v2ray_service(const char* config_file, int http_port, int socks_port);

/*
 * Removes the V2Ray systemd user service.
 *
 * Deletes the service file and reloads systemd to disable the service.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if service path initialization, file removal, or systemd reload/disable fails.
 */
int remove_v2ray_service();

/*
 * Starts the V2Ray systemd user service or a direct process in WSL.
 *
 * Launches the service or forks a process, sets proxy settings, and stores the process ID.
 *
 * Parameters:
 *   pid (pid_t*): Pointer to store the process ID of the started V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if service path initialization, forking, service start, or proxy setting fails.
 */
int start_v2ray_service(pid_t* pid);

/*
 * Stops the V2Ray systemd user service or direct process in WSL.
 *
 * Terminates the service or process and unsets proxy settings.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if service path initialization, stopping the service/process, or unsetting proxy fails.
 */
int stop_v2ray_service();

/*
 * Checks if the V2Ray service or process is running.
 *
 * Queries systemd service status or checks the PID file in WSL.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 1 if the service/process is running, 0 otherwise.
 *
 * Errors:
 *   Logs errors if service path initialization fails or a running process is detected.
 */
int is_v2ray_service_running();

/*
 * Sets system proxy environment variables for HTTP and SOCKS protocols.
 *
 * Configures proxy settings for the current process, parent shell, and a persistent environment file.
 *
 * Parameters:
 *   http_port (int): The port for the HTTP proxy.
 *   socks_port (int): The port for the SOCKS proxy.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if setting environment variables, writing the proxy file, or executing the shell command fails.
 */
int set_system_proxy(int http_port, int socks_port);

/*
 * Unsets system proxy environment variables.
 *
 * Clears proxy settings for the current process, parent shell, and updates the environment file.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if writing the proxy file or executing the unset command fails.
 */
int unset_system_proxy();

#endif