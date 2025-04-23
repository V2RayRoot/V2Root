#ifndef LIBV2ROOT_UTILS_H
#define LIBV2ROOT_UTILS_H

#include "libv2root_common.h"

/*
 * Validates an IP address or hostname.
 *
 * Checks if the address resolves to a valid IPv4 address using getaddrinfo.
 *
 * Parameters:
 *   address (const char*): The IP address or hostname to validate.
 *
 * Returns:
 *   int: 1 if the address is valid, 0 if it is NULL, empty, or invalid.
 *
 * Errors:
 *   None
 */
EXPORT int validate_address(const char* address);

/*
 * Validates a port number.
 *
 * Ensures the port string is a valid integer between 1 and 65535.
 *
 * Parameters:
 *   port (const char*): The port number as a string.
 *
 * Returns:
 *   int: 1 if the port is valid, 0 if it is NULL, empty, non-numeric, or out of range.
 *
 * Errors:
 *   None
 */
EXPORT int validate_port(const char* port);

/*
 * Logs a message to the v2root.log file with timestamp and context.
 *
 * Appends a log entry with the message, file, line, error code, and optional extra information.
 *
 * Parameters:
 *   message (const char*): The log message.
 *   file (const char*): The source file where the log is generated.
 *   line (int): The line number in the source file.
 *   error_code (int): The error code (0 for INFO logs).
 *   extra_info (const char*): Optional additional information.
 *
 * Returns:
 *   None
 *
 * Errors:
 *   Silently fails if the log file cannot be opened.
 */
EXPORT void log_message(const char* message, const char* file, int line, int error_code, const char* extra_info);

/*
 * Retrieves the current working directory.
 *
 * Uses platform-specific functions to get the current directory path.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   const char*: The current directory path, or an empty string on failure.
 *
 * Errors:
 *   Logs an error if the current directory cannot be retrieved.
 */
EXPORT const char* get_current_dir(void);

#endif