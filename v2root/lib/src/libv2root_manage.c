#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <jansson.h>
#include <stdint.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include "libv2root_win.h"
#define start_v2ray_process win_start_v2ray_process
#define stop_v2ray_process win_stop_v2ray_process
#define test_connection win_test_connection
#define ACCESS _access
#define SLEEP(ms) Sleep(ms)
#else
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "libv2root_linux.h"
#include "libv2root_service.h"
#define stop_v2ray_process linux_stop_v2ray_process
#define test_connection linux_test_connection
#define ACCESS access
#define SLEEP(ms) usleep((ms) * 1000)
#endif

#include "libv2root_common.h"
#include "libv2root_manage.h"
#include "libv2root_vless.h"
#include "libv2root_vmess.h"
#include "libv2root_shadowsocks.h"
#include "libv2root_utils.h"

static PID_TYPE v2ray_pid = 0;
static char v2ray_config_file[1024];
static char v2ray_executable_path[1024];

/*
 * Checks if the system is running under Windows Subsystem for Linux (WSL).
 *
 * Reads the /proc/version file to detect Microsoft or WSL strings, indicating a WSL environment.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 1 if running in WSL, 0 otherwise.
 *
 * Errors:
 *   Returns 0 if the /proc/version file cannot be opened.
 */
#ifndef _WIN32
static int is_wsl() {
    FILE* fp = fopen("/proc/version", "r");
    if (!fp) return 0;
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "Microsoft") || strstr(buffer, "WSL")) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}
#endif

/*
 * Decodes a base64-encoded string.
 *
 * Filters out invalid characters, validates the input length, and decodes the base64 string into a null-terminated string.
 * Logs the decoded output for debugging.
 *
 * Parameters:
 *   input (const char*): The base64-encoded string to decode.
 *
 * Returns:
 *   char*: A pointer to the decoded string on success, NULL on failure.
 *
 * Errors:
 *   Logs errors for null input, memory allocation failures, invalid base64 length, or invalid characters.
 *   Frees allocated memory and returns NULL on failure.
 */
static char* base64_decode(const char* input) {
    if (!input) {
        log_message("Null input for base64 decode", __FILE__, __LINE__, 0, NULL);
        return NULL;
    }
    size_t len = strlen(input);
    size_t clean_len = 0;
    char* clean_input = malloc(len + 1);
    if (!clean_input) {
        log_message("Failed to allocate memory for clean base64 input", __FILE__, __LINE__, 0, NULL);
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        if (isalnum(input[i]) || input[i] == '+' || input[i] == '/' || input[i] == '=') {
            clean_input[clean_len++] = input[i];
        }
    }
    clean_input[clean_len] = '\0';
    if (clean_len % 4 != 0) {
        log_message("Invalid base64 length", __FILE__, __LINE__, 0, clean_input);
        free(clean_input);
        return NULL;
    }
    size_t padding = 0;
    if (clean_len > 0 && clean_input[clean_len - 1] == '=') padding++;
    if (clean_len > 1 && clean_input[clean_len - 2] == '=') padding++;
    size_t out_len = ((clean_len * 3) / 4) - padding;
    char* output = malloc(out_len + 1);
    if (!output) {
        log_message("Failed to allocate memory for base64 decode", __FILE__, __LINE__, 0, NULL);
        free(clean_input);
        return NULL;
    }
    size_t i, j;
    for (i = 0, j = 0; i < clean_len; i += 4) {
        uint32_t val = 0;
        for (int k = 0; k < 4 && i + k < clean_len; k++) {
            char c = clean_input[i + k];
            if (c >= 'A' && c <= 'Z') val = (val << 6) | (c - 'A');
            else if (c >= 'a' && c <= 'z') val = (val << 6) | (c - 'a' + 26);
            else if (c >= '0' && c <= '9') val = (val << 6) | (c - '0' + 52);
            else if (c == '+') val = (val << 6) | 62;
            else if (c == '/') val = (val << 6) | 63;
            else if (c == '=') continue;
            else {
                log_message("Invalid base64 character", __FILE__, __LINE__, 0, clean_input);
                free(output);
                free(clean_input);
                return NULL;
            }
        }
        if (j + 2 < out_len) output[j++] = (val >> 16) & 0xFF;
        if (j + 1 < out_len) output[j++] = (val >> 8) & 0xFF;
        if (j < out_len) output[j++] = val & 0xFF;
    }
    output[j] = '\0';
    char debug_msg[512];
    snprintf(debug_msg, sizeof(debug_msg), "Base64 decoded: %s", output);
    log_message(debug_msg, __FILE__, __LINE__, 0, NULL);
    free(clean_input);
    return output;
}

/*
 * Initializes the V2Ray environment with configuration and executable paths.
 *
 * Validates the provided paths, checks if the executable exists, and stores the paths for later use.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   v2ray_path (const char*): Path to the V2Ray executable.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for null or overly long paths, or if the executable is not found.
 */
EXPORT int init_v2ray(const char* config_file, const char* v2ray_path) {
    if (!config_file || !v2ray_path) {
        log_message("Invalid config file or V2Ray path", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    if (strlen(config_file) >= sizeof(v2ray_config_file)) {
        log_message("Config file path too long", __FILE__, __LINE__, 0, config_file);
        return -1;
    }
    if (strlen(v2ray_path) >= sizeof(v2ray_executable_path)) {
        log_message("V2Ray executable path too long", __FILE__, __LINE__, 0, v2ray_path);
        return -1;
    }
    if (ACCESS(v2ray_path, F_OK) == -1) {
        log_message("V2Ray executable not found", __FILE__, __LINE__, errno, v2ray_path);
        return -1;
    }
    strncpy(v2ray_config_file, config_file, sizeof(v2ray_config_file) - 1);
    v2ray_config_file[sizeof(v2ray_config_file) - 1] = '\0';
    strncpy(v2ray_executable_path, v2ray_path, sizeof(v2ray_executable_path) - 1);
    v2ray_executable_path[sizeof(v2ray_executable_path) - 1] = '\0';
    log_message("V2Ray initialized with config and executable", __FILE__, __LINE__, 0, v2ray_path);
    return 0;
}

/*
 * Resets the system proxy settings.
 *
 * Disables the system proxy configuration for the current platform (Windows or Linux).
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, platform-specific error code on failure.
 *
 * Errors:
 *   Platform-specific functions handle and log errors internally.
 */
EXPORT int reset_network_proxy() {
#ifdef _WIN32
    return win_disable_system_proxy();
#else
    return linux_reset_network_proxy();
#endif
}

/*
 * Starts the V2Ray process with specified HTTP and SOCKS ports.
 *
 * Calls start_v2ray_with_pid to start the process and returns the process ID.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port (defaults to 2300 if <= 0).
 *   socks_port (int): SOCKS proxy port (defaults to 2301 if <= 0).
 *
 * Returns:
 *   int: Process ID on success, -1 on failure.
 *
 * Errors:
 *   Delegates error handling to start_v2ray_with_pid.
 */
EXPORT int start_v2ray(int http_port, int socks_port) {
    PID_TYPE pid;
    int result = start_v2ray_with_pid(http_port, socks_port, &pid);
    if (result == 0) {
        return (int)pid;
    }
    return result;
}

/*
 * Starts the V2Ray process with specified ports and stores the process ID.
 *
 * Validates initialization, configuration file, and starts the V2Ray process using platform-specific functions.
 * Enables system proxy and logs the process ID.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port (defaults to 2300 if <= 0).
 *   socks_port (int): SOCKS proxy port (defaults to 2301 if <= 0).
 *   pid (PID_TYPE*): Pointer to store the process ID.
 *
 * Returns:
 *   int: 0 on success, -1 on failure, -4 if config file is missing.
 *
 * Errors:
 *   Logs errors for uninitialized V2Ray, missing config file, or platform-specific failures.
 */
EXPORT int start_v2ray_with_pid(int http_port, int socks_port, PID_TYPE* pid) {
    if (v2ray_config_file[0] == '\0' || v2ray_executable_path[0] == '\0') {
        log_message("V2Ray not initialized", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    if (ACCESS(v2ray_config_file, F_OK) == -1) {
        log_message("Config file not found for V2Ray start", __FILE__, __LINE__, errno, v2ray_config_file);
        return -4;
    }
    if (http_port <= 0) http_port = 2300;
    if (socks_port <= 0) socks_port = 2301;
    char port_info[256];
    snprintf(port_info, sizeof(port_info), "Starting V2Ray with HTTP Port: %d, SOCKS Port: %d", http_port, socks_port);
    log_message(port_info, __FILE__, __LINE__, 0, NULL);
#ifdef _WIN32
    if (win_enable_system_proxy(http_port, socks_port) != 0) {
        log_message("Failed to enable system proxy in Windows", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    if (start_v2ray_process(v2ray_config_file, v2ray_executable_path, &v2ray_pid) != 0) {
        log_message("Failed to start V2Ray process in Windows", __FILE__, __LINE__, 0, NULL);
        win_disable_system_proxy();
        return -1;
    }
    save_pid_to_registry(v2ray_pid);
    *pid = v2ray_pid;
#else
    if (is_wsl()) {
        if (linux_enable_system_proxy(http_port, socks_port) != 0) {
            log_message("Failed to enable system proxy in WSL", __FILE__, __LINE__, 0, NULL);
            return -1;
        }
        if (linux_start_v2ray_process(v2ray_config_file, pid) != 0) {
            log_message("Failed to start V2Ray process in WSL", __FILE__, __LINE__, 0, NULL);
            linux_disable_system_proxy();
            return -1;
        }
        v2ray_pid = *pid;
    } else {
        if (create_v2ray_service(v2ray_config_file, http_port, socks_port) != 0) {
            log_message("Failed to create V2Ray service in Linux", __FILE__, __LINE__, 0, NULL);
            return -1;
        }
        if (start_v2ray_service(pid) != 0) {
            log_message("Failed to start V2Ray service in Linux", __FILE__, __LINE__, 0, NULL);
            remove_v2ray_service();
            return -1;
        }
        if (linux_enable_system_proxy(http_port, socks_port) != 0) {
            log_message("Failed to enable system proxy in Linux", __FILE__, __LINE__, 0, NULL);
            stop_v2ray_service();
            remove_v2ray_service();
            return -1;
        }
        v2ray_pid = *pid;
    }
#endif
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "V2Ray started with PID: %lu", (unsigned long)v2ray_pid);
    log_message("V2Ray started successfully", __FILE__, __LINE__, 0, extra_info);
    return 0;
}

/*
 * Stops the running V2Ray process.
 *
 * Terminates the V2Ray process using platform-specific functions and disables the system proxy.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if V2Ray is not initialized or if stopping the process fails.
 */
EXPORT int stop_v2ray() {
    if (v2ray_config_file[0] == '\0') {
        log_message("V2Ray not initialized", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
#ifdef _WIN32
    PID_TYPE pid_from_registry = load_pid_from_registry();
    if (pid_from_registry == 0) {
        log_message("No V2Ray process found in registry", __FILE__, __LINE__, 0, NULL);
        win_disable_system_proxy();
        return 0;
    }
    if (win_stop_v2ray_process(pid_from_registry) == 0) {
        v2ray_pid = 0;
        win_disable_system_proxy();
        log_message("V2Ray process stopped successfully", __FILE__, __LINE__, 0, NULL);
        return 0;
    } else {
        log_message("Failed to stop V2Ray process", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
#else
    if (is_wsl()) {
        if (linux_stop_v2ray_process(v2ray_pid) != 0) {
            log_message("Failed to stop V2Ray process in WSL", __FILE__, __LINE__, 0, NULL);
            return -1;
        }
        linux_disable_system_proxy();
    } else {
        if (stop_v2ray_service() == 0) {
            remove_v2ray_service();
            log_message("V2Ray service stopped successfully", __FILE__, __LINE__, 0, NULL);
        } else {
            log_message("Failed to stop V2Ray service", __FILE__, __LINE__, 0, NULL);
            return -1;
        }
        linux_reset_network_proxy();
    }
#endif
    v2ray_pid = 0;
    return 0;
}

/*
 * Parses a V2Ray configuration string and writes it to the configuration file.
 *
 * Supports VLESS, VMess, and Shadowsocks protocols, writing the parsed configuration to the file specified in init_v2ray.
 *
 * Parameters:
 *   config_str (const char*): The configuration string to parse.
 *   http_port (int): HTTP proxy port (defaults to 2300 if <= 0).
 *   socks_port (int): SOCKS proxy port (defaults to 2301 if <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for null input, file opening failures, unknown protocols, or parsing failures.
 */
EXPORT int parse_config_string(const char* config_str, int http_port, int socks_port) {
    if (config_str == NULL) {
        log_message("Null config string", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    if (http_port <= 0) {
        http_port = 2300;
        log_message("No HTTP port provided for config parsing, using default", __FILE__, __LINE__, 0, "2300");
    }
    if (socks_port <= 0) {
        socks_port = 2301;
        log_message("No SOCKS port provided for config parsing, using default", __FILE__, __LINE__, 0, "2301");
    }
    FILE* fp = fopen(v2ray_config_file, "w");
    if (!fp) {
        log_message("Failed to open config file", __FILE__, __LINE__, errno, v2ray_config_file);
        return -1;
    }
    int result = -1;
    if (strncmp(config_str, "vless://", 8) == 0) {
        result = parse_vless_string(config_str, fp, http_port, socks_port);
    } else if (strncmp(config_str, "vmess://", 8) == 0) {
        result = parse_vmess_string(config_str, fp, http_port, socks_port);
    } else if (strncmp(config_str, "ss://", 5) == 0) {
        result = parse_shadowsocks_string(config_str, fp, http_port, socks_port);
    } else {
        fclose(fp);
        log_message("Unknown protocol", __FILE__, __LINE__, 0, config_str);
        return -1;
    }
    fclose(fp);
    if (result != 0) {
        log_message("Config parsing failed", __FILE__, __LINE__, result, config_str);
        return -1;
    }
    return 0;
}

/*
 * Tests a V2Ray configuration by starting a temporary process and measuring latency.
 *
 * Parses the configuration string, starts a V2Ray process, and tests the connection latency.
 * Supports VLESS, VMess, and Shadowsocks protocols.
 *
 * Parameters:
 *   config_str (const char*): The configuration string to test.
 *   latency (int*): Pointer to store the measured latency in milliseconds.
 *   http_port (int): HTTP proxy port (defaults to 2300 if <= 0).
 *   socks_port (int): SOCKS proxy port (defaults to 2301 if <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure, -2 if the V2Ray process fails to start.
 *
 * Errors:
 *   Logs errors for null inputs, invalid configurations, JSON parsing failures, or process failures.
 *   Skips invalid VMess configurations and continues with other protocols.
 */
EXPORT int test_config_connection(const char* config_str, int* latency, int http_port, int socks_port) {
    if (config_str == NULL || latency == NULL) {
        log_message("Null config string or latency pointer", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    if (http_port <= 0) {
        http_port = 2300;
        log_message("No HTTP port provided for test, using default", __FILE__, __LINE__, 0, "2300");
    }
    if (socks_port <= 0) {
        socks_port = 2301;
        log_message("No SOCKS port provided for test, using default", __FILE__, __LINE__, 0, "2301");
    }
    char address[2048] = "";
    char port_str[16] = "";
    if (strncmp(config_str, "vless://", 8) == 0) {
        const char* at_sign = strchr(config_str, '@');
        if (!at_sign) {
            log_message("No @ found in VLESS config string", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        const char* colon = strchr(at_sign + 1, ':');
        if (!colon) {
            log_message("No port found in VLESS config string", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        const char* question_mark = strchr(colon, '?');
        size_t addr_len = colon - (at_sign + 1);
        if (addr_len >= sizeof(address)) {
            log_message("Address too long in VLESS config", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        strncpy(address, at_sign + 1, addr_len);
        address[addr_len] = '\0';
        size_t port_len = (question_mark ? question_mark : strchr(colon, '\0')) - (colon + 1);
        if (port_len >= sizeof(port_str)) {
            log_message("Port too long in VLESS config", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        strncpy(port_str, colon + 1, port_len);
        port_str[port_len] = '\0';
    } else if (strncmp(config_str, "vmess://", 8) == 0) {
        const char* base64_str = config_str + 8;
        char debug_msg[512];
        snprintf(debug_msg, sizeof(debug_msg), "Processing VMess config: %s", config_str);
        log_message(debug_msg, __FILE__, __LINE__, 0, NULL);
        char* decoded = base64_decode(base64_str);
        if (!decoded) {
            log_message("Failed to decode VMess base64, skipping VMess config", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        int is_valid_utf8 = 1;
        for (size_t i = 0; decoded[i]; i++) {
            if ((unsigned char)decoded[i] >= 0x80 && (unsigned char)decoded[i] <= 0xBF) {
                is_valid_utf8 = 0;
                break;
            }
        }
        if (!is_valid_utf8) {
            log_message("Decoded VMess string is not valid UTF-8, skipping", __FILE__, __LINE__, 0, decoded);
            free(decoded);
            return -1;
        }
        json_error_t error;
        json_t* json = json_loads(decoded, 0, &error);
        if (!json) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "JSON error: %s (line %d, column %d)", error.text, error.line, error.column);
            log_message("Failed to parse VMess JSON, skipping VMess config", __FILE__, __LINE__, 0, err_msg);
            free(decoded);
            return -1;
        }
        const char* addr = json_string_value(json_object_get(json, "add"));
        int port = json_integer_value(json_object_get(json, "port"));
        if (!addr || port <= 0) {
            log_message("Missing address or port in VMess JSON, skipping", __FILE__, __LINE__, 0, config_str);
            json_decref(json);
            free(decoded);
            return -1;
        }
        strncpy(address, addr, sizeof(address) - 1);
        address[sizeof(address) - 1] = '\0';
        snprintf(port_str, sizeof(port_str), "%d", port);
        json_decref(json);
        free(decoded);
    } else if (strncmp(config_str, "ss://", 5) == 0) {
        const char* at_sign = strchr(config_str, '@');
        if (!at_sign) {
            log_message("Invalid Shadowsocks config format", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        const char* colon = strchr(at_sign + 1, ':');
        if (!colon) {
            log_message("No port found in Shadowsocks config", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        const char* hash = strchr(colon, '#');
        size_t addr_len = colon - (at_sign + 1);
        if (addr_len >= sizeof(address)) {
            log_message("Address too long in Shadowsocks config", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        strncpy(address, at_sign + 1, addr_len);
        address[addr_len] = '\0';
        size_t port_len = (hash ? hash : strchr(colon, '\0')) - (colon + 1);
        if (port_len >= sizeof(port_str)) {
            log_message("Port too long in Shadowsocks config", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        strncpy(port_str, colon + 1, port_len);
        port_str[port_len] = '\0';
    } else {
        log_message("Unknown protocol in test", __FILE__, __LINE__, 0, config_str);
        return -1;
    }
    char addr_info[256];
    snprintf(addr_info, sizeof(addr_info), "Extracted address: %s, port: %s", address, port_str);
    log_message(addr_info, __FILE__, __LINE__, 0, NULL);
    if (!validate_address(address)) {
        log_message("Invalid address in config", __FILE__, __LINE__, 0, address);
        return -1;
    }
    if (!validate_port(port_str)) {
        log_message("Invalid port in config", __FILE__, __LINE__, 0, port_str);
        return -1;
    }
    FILE* fp = fopen("config_test.json", "w");
    if (!fp) {
        log_message("Failed to open config_test.json", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    int parse_result = -1;
    if (strncmp(config_str, "vless://", 8) == 0) {
        log_message("Parsing VLESS config", __FILE__, __LINE__, 0, config_str);
        parse_result = parse_vless_string(config_str, fp, http_port, socks_port);
    } else if (strncmp(config_str, "vmess://", 8) == 0) {
        log_message("Parsing VMess config", __FILE__, __LINE__, 0, config_str);
        parse_result = parse_vmess_string(config_str, fp, http_port, socks_port);
    } else if (strncmp(config_str, "ss://", 5) == 0) {
        log_message("Parsing Shadowsocks config", __FILE__, __LINE__, 0, config_str);
        parse_result = parse_shadowsocks_string(config_str, fp, http_port, socks_port);
    }
    fclose(fp);
    if (parse_result != 0) {
        log_message("Test config parsing failed", __FILE__, __LINE__, parse_result, config_str);
        return -1;
    }
    PID_TYPE test_pid = 0;
#ifdef _WIN32
    if (start_v2ray_process("config_test.json", v2ray_executable_path, &test_pid) != 0) {
        log_message("Failed to start V2Ray process for test", __FILE__, __LINE__, 0, NULL);
        return -2;
    }
#else
    if (linux_start_v2ray_process("config_test.json", &test_pid) != 0) {
        log_message("Failed to start V2Ray process for test", __FILE__, __LINE__, 0, NULL);
        return -2;
    }
#endif
    if (test_pid == 0) {
        log_message("Invalid PID returned from start_v2ray_process", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    SLEEP(2000);
    int result = -1;
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, test_pid);
    if (hProcess == NULL) {
        DWORD error = GetLastError();
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to open V2Ray process for termination (PID: %lu)", (unsigned long)test_pid);
        log_message(err_msg, __FILE__, __LINE__, error, NULL);
        stop_v2ray_process(test_pid);
        return -1;
    }
    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
        char extra_info[256];
        snprintf(extra_info, sizeof(extra_info), "V2Ray exited with code: %lu", exitCode);
        log_message("V2Ray process exited prematurely", __FILE__, __LINE__, 0, extra_info);
        CloseHandle(hProcess);
        stop_v2ray_process(test_pid);
        return -1;
    }
    result = test_connection(http_port, latency, hProcess);
    stop_v2ray_process(test_pid);
    CloseHandle(hProcess);
#else
    int status;
    if (waitpid(test_pid, &status, WNOHANG) == test_pid) {
        char extra_info[256];
        snprintf(extra_info, sizeof(extra_info), "V2Ray exited with code: %d", WEXITSTATUS(status));
        log_message("V2Ray process exited prematurely", __FILE__, __LINE__, 0, extra_info);
        stop_v2ray_process(test_pid);
        return -1;
    }
    result = test_connection(http_port, socks_port, latency, test_pid);
    stop_v2ray_process(test_pid);
#endif
    if (unlink("config_test.json") != 0) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to delete config_test.json, errno: %d", errno);
        log_message(err_msg, __FILE__, __LINE__, errno, NULL);
    }
    return result;
}