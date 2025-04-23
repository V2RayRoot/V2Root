#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
    #include <netdb.h>
    #include <sys/socket.h>
    #include <errno.h>
    #include <arpa/inet.h>
#endif
#include "libv2root_utils.h"

/*
 * Validates an IP address or hostname.
 *
 * Uses getaddrinfo to check if the address resolves to a valid IPv4 address.
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
int validate_address(const char* address) {
    if (address == NULL || strlen(address) == 0) return 0;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(address, NULL, &hints, &res);
    if (status != 0) {
        return 0;
    }
    freeaddrinfo(res);
    return 1;
}

/*
 * Validates a port number.
 *
 * Checks if the port string is a valid integer between 1 and 65535.
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
int validate_port(const char* port) {
    if (port == NULL || strlen(port) == 0) return 0;

    int port_num = atoi(port);
    if (port_num <= 0 || port_num > 65535) return 0;

    for (size_t i = 0; i < strlen(port); i++) {
        if (!isdigit(port[i])) return 0;
    }
    return 1;
}

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
void log_message(const char* message, const char* file, int line, int error_code, const char* extra_info) {
    FILE* log_file = fopen("v2root.log", "a");
    if (!log_file) return;

    char timestamp[20];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [%s] %s:%d - %s", timestamp, error_code ? "ERROR" : "INFO", file, line, message);
    if (error_code) fprintf(log_file, " (Error: %d)", error_code);
    if (extra_info) fprintf(log_file, " [%s]", extra_info);
    fprintf(log_file, "\n");

    fclose(log_file);
}

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
const char* get_current_dir() {
#ifdef _WIN32
    static char dir[MAX_PATH] = {0};
    if (GetCurrentDirectoryA(MAX_PATH, dir) == 0) {
        log_message("Failed to get current directory", __FILE__, __LINE__, GetLastError(), NULL);
        return "";
    }
    return dir;
#else
    static char dir[PATH_MAX] = {0};
    if (getcwd(dir, PATH_MAX) == NULL) {
        log_message("Failed to get current directory", __FILE__, __LINE__, errno, NULL);
        return "";
    }
    return dir;
#endif
}