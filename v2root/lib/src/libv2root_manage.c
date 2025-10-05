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

/* Forward declarations */
static char* base64_decode(const char* input);
#ifndef _WIN32
static int is_wsl(void);
#endif

/* Declare start_v2ray_with_pid before start_v2ray uses it */
EXPORT int start_v2ray_with_pid(int http_port, int socks_port, PID_TYPE* pid);

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
 * On Windows: Validates and stores the user-provided v2ray_path.
 * On Linux: Validates that v2ray exists in system PATH (ignores v2ray_path if provided).
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   v2ray_path (const char*): Path to the V2Ray executable (Windows only; ignored on Linux).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for null or overly long paths, or if the executable is not found.
 */
EXPORT int init_v2ray(const char* config_file, const char* v2ray_path) {
    if (!config_file) {
        log_message("Invalid config file", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
#ifdef _WIN32
    /* Windows: Require user-provided v2ray_path */
    if (!v2ray_path) {
        log_message("V2Ray path is required on Windows", __FILE__, __LINE__, 0, NULL);
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
    strncpy(v2ray_executable_path, v2ray_path, sizeof(v2ray_executable_path) - 1);
    v2ray_executable_path[sizeof(v2ray_executable_path) - 1] = '\0';
#else
    /* Linux: Ignore v2ray_path, always use system-installed v2ray */
    if (v2ray_path) {
        log_message("v2ray_path ignored on Linux - using system-installed V2Ray", __FILE__, __LINE__, 0, v2ray_path);
    }
    
    /* Store "v2ray" as the executable path - system will find it via PATH */
    strncpy(v2ray_executable_path, "v2ray", sizeof(v2ray_executable_path) - 1);
    v2ray_executable_path[sizeof(v2ray_executable_path) - 1] = '\0';
    
    /* Verify v2ray is accessible in PATH */
    if (system("which v2ray > /dev/null 2>&1") != 0) {
        log_message("V2Ray not found in system PATH - install via package manager (apt/dnf/pacman)", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
#endif
    
    if (strlen(config_file) >= sizeof(v2ray_config_file)) {
        log_message("Config file path too long", __FILE__, __LINE__, 0, config_file);
        return -1;
    }
    
    strncpy(v2ray_config_file, config_file, sizeof(v2ray_config_file) - 1);
    v2ray_config_file[sizeof(v2ray_config_file) - 1] = '\0';
    
    log_message("V2Ray initialized with config and executable", __FILE__, __LINE__, 0, v2ray_executable_path);
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

        size_t addr_len = colon - (at_sign + 1);
        if (addr_len >= sizeof(address)) {
            log_message("Address too long in Shadowsocks config", __FILE__, __LINE__, 0, config_str);
            return -1;
        }
        strncpy(address, at_sign + 1, addr_len);
        address[addr_len] = '\0';
    
        const char* port_end = colon + 1;
        while (isdigit(*port_end)) {
            port_end++;
        }
        size_t port_len = port_end - (colon + 1);
        if (port_len == 0 || port_len >= sizeof(port_str)) {
            log_message("Port too long or invalid in Shadowsocks config", __FILE__, __LINE__, 0, config_str);
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

/*
 * Pings a server to measure network latency.
 *
 * Creates a TCP connection to the specified address and port, measures the time taken to establish
 * the connection, and returns the latency in milliseconds. This function is useful for testing
 * network connectivity and measuring round-trip time to a server.
 *
 * Parameters:
 *   address (const char*): The server address (IP or hostname) to ping. Supports IPv4, IPv6, and domain names.
 *   port (int): The port number to connect to (1-65535).
 *
 * Returns:
 *   int: Latency in milliseconds on success, -1 on failure (connection error, invalid address, or invalid port).
 *
 * Errors:
 *   - Logs errors for null address, invalid port, address resolution failures, socket creation failures,
 *     or connection failures.
 *   - Returns -1 if the address cannot be resolved or the connection times out.
 *   - Platform-specific error codes are logged (WSAGetLastError on Windows, errno on Linux).
 *
 * Notes:
 *   - The function uses a non-blocking socket with a timeout for the connection attempt.
 *   - The latency measurement includes DNS resolution time and TCP handshake time.
 *   - On Windows, requires WSA initialization (handled automatically within the function).
 */
EXPORT int ping_server(const char* address, int port) {
    if (!address || address[0] == '\0') {
        log_message("Null or empty address for ping", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    if (port <= 0 || port > 65535) {
        log_message("Invalid port for ping", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    if (!validate_address(address)) {
        log_message("Invalid address format for ping", __FILE__, __LINE__, 0, address);
        return -1;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log_message("WSAStartup failed", __FILE__, __LINE__, WSAGetLastError(), NULL);
        return -1;
    }

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);

    struct addrinfo hints, *result = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    QueryPerformanceCounter(&start);

    if (getaddrinfo(address, port_str, &hints, &result) != 0) {
        log_message("Failed to resolve address", __FILE__, __LINE__, WSAGetLastError(), address);
        WSACleanup();
        return -1;
    }

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        log_message("Failed to create socket", __FILE__, __LINE__, WSAGetLastError(), NULL);
        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }

    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        DWORD error = WSAGetLastError();
        log_message("Failed to connect to server", __FILE__, __LINE__, error, address);
        closesocket(sock);
        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }

    QueryPerformanceCounter(&end);

    /* Calculate latency with microsecond precision */
    double elapsed_ms = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / (double)freq.QuadPart;
    int latency = (int)(elapsed_ms + 0.5);
    
    /* Ensure minimum latency of 1ms */
    if (latency < 1) {
        latency = 1;
    }

    closesocket(sock);
    freeaddrinfo(result);
    WSACleanup();

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Ping to %s:%d successful, latency: %d ms (actual: %.2f ms)", address, port, latency, elapsed_ms);
    log_message("Ping successful", __FILE__, __LINE__, 0, extra_info);

    return latency;

#else
    struct timeval start, end;
    struct addrinfo hints, *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    gettimeofday(&start, NULL);

    if (getaddrinfo(address, port_str, &hints, &result) != 0) {
        log_message("Failed to resolve address", __FILE__, __LINE__, errno, address);
        return -1;
    }

    int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock < 0) {
        log_message("Failed to create socket", __FILE__, __LINE__, errno, NULL);
        freeaddrinfo(result);
        return -1;
    }

    if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
        log_message("Failed to connect to server", __FILE__, __LINE__, errno, address);
        close(sock);
        freeaddrinfo(result);
        return -1;
    }

    gettimeofday(&end, NULL);

    /* Calculate latency with microsecond precision */
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed_ms = (seconds * 1000.0) + (microseconds / 1000.0);
    
    int latency = (int)(elapsed_ms + 0.5);
    
    /* Ensure minimum latency of 1ms */
    if (latency < 1) {
        latency = 1;
    }

    close(sock);
    freeaddrinfo(result);

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Ping to %s:%d successful, latency: %d ms (actual: %.2f ms)", address, port, latency, elapsed_ms);
    log_message("Ping successful", __FILE__, __LINE__, 0, extra_info);

    return latency;
#endif
}

/*
 * Performs quick lightweight probe (DNS + TCP only).
 * Used for fast filtering before full probe.
 */
EXPORT int probe_config_quick(const char* config_str, ProbeResult* result, int http_port, int socks_port) {
    if (!config_str || !result) {
        log_message("Null config or result pointer for quick probe", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    /* Initialize result */
    memset(result, 0, sizeof(ProbeResult));
    result->attempts = 1;
    strncpy(result->error_type, PROBE_ERROR_NONE, sizeof(result->error_type) - 1);
    
    /* Extract address and port from config string */
    char address[2048] = "";
    char port_str[16] = "";
    
    /* Parse based on protocol (similar to test_config_connection) */
    if (strncmp(config_str, "vless://", 8) == 0) {
        const char* at_sign = strchr(config_str, '@');
        if (!at_sign) {
            strncpy(result->error_type, PROBE_ERROR_UNKNOWN, sizeof(result->error_type) - 1);
            return -1;
        }
        const char* colon = strchr(at_sign + 1, ':');
        if (!colon) {
            strncpy(result->error_type, PROBE_ERROR_UNKNOWN, sizeof(result->error_type) - 1);
            return -1;
        }
        const char* question_mark = strchr(colon, '?');
        size_t addr_len = colon - (at_sign + 1);
        if (addr_len >= sizeof(address)) addr_len = sizeof(address) - 1;
        strncpy(address, at_sign + 1, addr_len);
        address[addr_len] = '\0';
        size_t port_len = (question_mark ? question_mark : strchr(colon, '\0')) - (colon + 1);
        if (port_len >= sizeof(port_str)) port_len = sizeof(port_str) - 1;
        strncpy(port_str, colon + 1, port_len);
        port_str[port_len] = '\0';
    } else {
        /* Add similar parsing for vmess/ss if needed */
        strncpy(result->error_type, PROBE_ERROR_UNKNOWN, sizeof(result->error_type) - 1);
        snprintf(result->error_details, sizeof(result->error_details), "Unsupported protocol for quick probe");
        return -1;
    }
    
    /* Perform DNS resolution with timing */
#ifdef _WIN32
    LARGE_INTEGER freq, dns_start, dns_end, tcp_start, tcp_end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&dns_start);
    
    struct addrinfo hints, *res = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(address, port_str, &hints, &res) != 0) {
        QueryPerformanceCounter(&dns_end);
        result->dns_ms = (int)(((dns_end.QuadPart - dns_start.QuadPart) * 1000) / freq.QuadPart);
        strncpy(result->error_type, PROBE_ERROR_DNS, sizeof(result->error_type) - 1);
        snprintf(result->error_details, sizeof(result->error_details), "DNS resolution failed for %s", address);
        return -1;
    }
    
    QueryPerformanceCounter(&dns_end);
    result->dns_ms = (int)(((dns_end.QuadPart - dns_start.QuadPart) * 1000) / freq.QuadPart);
    if (result->dns_ms < 1) result->dns_ms = 1;
    
    /* TCP connect with timing */
    QueryPerformanceCounter(&tcp_start);
    
    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(res);
        strncpy(result->error_type, PROBE_ERROR_TCP, sizeof(result->error_type) - 1);
        return -1;
    }
    
    /* Set timeout */
    DWORD timeout = DEFAULT_TCP_TIMEOUT_MS;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    
    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        QueryPerformanceCounter(&tcp_end);
        result->tcp_connect_ms = (int)(((tcp_end.QuadPart - tcp_start.QuadPart) * 1000) / freq.QuadPart);
        strncpy(result->error_type, PROBE_ERROR_TCP, sizeof(result->error_type) - 1);
        snprintf(result->error_details, sizeof(result->error_details), "TCP connect failed to %s:%s", address, port_str);
        closesocket(sock);
        freeaddrinfo(res);
        return -1;
    }
    
    QueryPerformanceCounter(&tcp_end);
    result->tcp_connect_ms = (int)(((tcp_end.QuadPart - tcp_start.QuadPart) * 1000) / freq.QuadPart);
    if (result->tcp_connect_ms < 1) result->tcp_connect_ms = 1;
    
    closesocket(sock);
    freeaddrinfo(res);
    
#else
    struct timeval dns_start, dns_end, tcp_start, tcp_end;
    gettimeofday(&dns_start, NULL);
    
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(address, port_str, &hints, &res) != 0) {
        gettimeofday(&dns_end, NULL);
        long sec = dns_end.tv_sec - dns_start.tv_sec;
        long usec = dns_end.tv_usec - dns_start.tv_usec;
        result->dns_ms = (int)((sec * 1000) + (usec / 1000));
        strncpy(result->error_type, PROBE_ERROR_DNS, sizeof(result->error_type) - 1);
        snprintf(result->error_details, sizeof(result->error_details), "DNS resolution failed for %s", address);
        return -1;
    }
    
    gettimeofday(&dns_end, NULL);
    long sec = dns_end.tv_sec - dns_start.tv_sec;
    long usec = dns_end.tv_usec - dns_start.tv_usec;
    result->dns_ms = (int)((sec * 1000) + (usec / 1000));
    if (result->dns_ms < 1) result->dns_ms = 1;
    
    /* TCP connect with timing */
    gettimeofday(&tcp_start, NULL);
    
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        strncpy(result->error_type, PROBE_ERROR_TCP, sizeof(result->error_type) - 1);
        return -1;
    }
    
    /* Set timeout */
    struct timeval timeout;
    timeout.tv_sec = DEFAULT_TCP_TIMEOUT_MS / 1000;
    timeout.tv_usec = (DEFAULT_TCP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        gettimeofday(&tcp_end, NULL);
        sec = tcp_end.tv_sec - tcp_start.tv_sec;
        usec = tcp_end.tv_usec - tcp_start.tv_usec;
        result->tcp_connect_ms = (int)((sec * 1000) + (usec / 1000));
        strncpy(result->error_type, PROBE_ERROR_TCP, sizeof(result->error_type) - 1);
        snprintf(result->error_details, sizeof(result->error_details), "TCP connect failed to %s:%s", address, port_str);
        close(sock);
        freeaddrinfo(res);
        return -1;
    }
    
    gettimeofday(&tcp_end, NULL);
    sec = tcp_end.tv_sec - tcp_start.tv_sec;
    usec = tcp_end.tv_usec - tcp_start.tv_usec;
    result->tcp_connect_ms = (int)((sec * 1000) + (usec / 1000));
    if (result->tcp_connect_ms < 1) result->tcp_connect_ms = 1;
    
    close(sock);
    freeaddrinfo(res);
#endif
    
    result->success = 1;
    result->total_ms = result->dns_ms + result->tcp_connect_ms;
    result->score = calculate_probe_score(result->total_ms, result->tcp_connect_ms, 1);
    
    char extra_info[512];
    snprintf(extra_info, sizeof(extra_info), 
             "Quick probe: DNS=%dms, TCP=%dms, Total=%dms, Score=%.3f",
             result->dns_ms, result->tcp_connect_ms, result->total_ms, result->score);
    log_message("Quick probe completed", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Performs full end-to-end probe including actual HTTP request through proxy.
 * This is the V2rayNG-style comprehensive test.
 */
EXPORT int probe_config_full(const char* config_str, ProbeResult* result, int http_port, int socks_port, int attempts) {
    if (!config_str || !result) {
        log_message("Null config or result pointer for full probe", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    if (attempts < 1) attempts = 1;
    if (attempts > 5) attempts = 5;
    
    /* Initialize result */
    memset(result, 0, sizeof(ProbeResult));
    result->attempts = attempts;
    strncpy(result->error_type, PROBE_ERROR_NONE, sizeof(result->error_type) - 1);
    
    /* Step 1: Quick pre-check (DNS + TCP) */
    ProbeResult quick_result;
    if (probe_config_quick(config_str, &quick_result, http_port, socks_port) != 0) {
        /* Quick check failed, copy results and return */
        memcpy(result, &quick_result, sizeof(ProbeResult));
        log_message("Quick probe failed, skipping full probe", __FILE__, __LINE__, 0, result->error_details);
        return -1;
    }
    
    /* Copy quick check results */
    result->dns_ms = quick_result.dns_ms;
    result->tcp_connect_ms = quick_result.tcp_connect_ms;
    
    /* Step 2: Full app-level probe through proxy */
    /* Reuse existing test_config_connection but capture more timing details */
    int latency = 0;
    int test_result = test_config_connection(config_str, &latency, http_port, socks_port);
    
    if (test_result != 0) {
        strncpy(result->error_type, PROBE_ERROR_TRANSPORT, sizeof(result->error_type) - 1);
        snprintf(result->error_details, sizeof(result->error_details), 
                 "Proxy connection test failed (code %d)", test_result);
        return -1;
    }
    
    /* Step 3: Record results */
    result->ttfb_ms = latency;
    result->proxy_setup_ms = latency; /* Approximation */
    result->total_ms = result->dns_ms + result->tcp_connect_ms + result->ttfb_ms;
    result->success = 1;
    result->score = calculate_probe_score(result->ttfb_ms, result->tcp_connect_ms, 1);
    
    char extra_info[512];
    snprintf(extra_info, sizeof(extra_info),
             "Full probe: DNS=%dms, TCP=%dms, TTFB=%dms, Total=%dms, Score=%.3f",
             result->dns_ms, result->tcp_connect_ms, result->ttfb_ms, 
             result->total_ms, result->score);
    log_message("Full probe completed successfully", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Performs a single HTTP request to measure TTFB through the proxy.
 * Returns a JSON string with platform, success, ttfb_ms, http_status, and error_message.
 */
EXPORT char* measure_ttfb(const char* config_str, int http_port) {
    static char result_buffer[1024];  /* Static buffer for return value */
    
    if (!config_str) {
        snprintf(result_buffer, sizeof(result_buffer),
                 "{\"platform\": \"unknown\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Null config string\"}");
        return result_buffer;
    }
    
    if (http_port <= 0) {
        http_port = DEFAULT_HTTP_PORT;
        log_message("No HTTP port provided for TTFB test, using default", __FILE__, __LINE__, 0, "2300");
    }
    
    /* Write config to a temporary file */
    FILE* fp = fopen("ttfb_test_config.json", "w");
    if (!fp) {
        snprintf(result_buffer, sizeof(result_buffer),
                 "{\"platform\": \"unknown\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to create temp config file\"}");
        return result_buffer;
    }
    
    /* Parse the configuration */
    int parse_result = -1;
    if (strncmp(config_str, "vless://", 8) == 0) {
        parse_result = parse_vless_string(config_str, fp, http_port, DEFAULT_SOCKS_PORT);
    } else if (strncmp(config_str, "vmess://", 8) == 0) {
        parse_result = parse_vmess_string(config_str, fp, http_port, DEFAULT_SOCKS_PORT);
    } else if (strncmp(config_str, "ss://", 5) == 0) {
        parse_result = parse_shadowsocks_string(config_str, fp, http_port, DEFAULT_SOCKS_PORT);
    }
    fclose(fp);
    
    if (parse_result != 0) {
        unlink("ttfb_test_config.json");
        snprintf(result_buffer, sizeof(result_buffer),
                 "{\"platform\": \"unknown\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to parse configuration\"}");
        return result_buffer;
    }
    
    /* Start V2Ray process with the config */
    PID_TYPE pid = 0;
    char* ttfb_result;
    
#ifdef _WIN32
    if (start_v2ray_process("ttfb_test_config.json", v2ray_executable_path, &pid) != 0) {
        unlink("ttfb_test_config.json");
        snprintf(result_buffer, sizeof(result_buffer),
                 "{\"platform\": \"windows\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to start V2Ray process\"}");
        return result_buffer;
    }
    SLEEP(2000); /* Wait for V2Ray to initialize */
    ttfb_result = win_measure_ttfb(http_port);
    
    /* Copy result to static buffer before stopping process */
    strncpy(result_buffer, ttfb_result, sizeof(result_buffer) - 1);
    result_buffer[sizeof(result_buffer) - 1] = '\0';
    
    stop_v2ray_process(pid);
#else
    if (linux_start_v2ray_process("ttfb_test_config.json", &pid) != 0) {
        unlink("ttfb_test_config.json");
        snprintf(result_buffer, sizeof(result_buffer),
                 "{\"platform\": \"linux\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to start V2Ray process\"}");
        return result_buffer;
    }
    
    SLEEP(2000); /* Wait for V2Ray to initialize */
    
    /* Check if process is still running */
    int status;
    if (waitpid(pid, &status, WNOHANG) == pid) {
        unlink("ttfb_test_config.json");
        snprintf(result_buffer, sizeof(result_buffer),
                 "{\"platform\": \"linux\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"V2Ray process exited prematurely\"}");
        return result_buffer;
    }
    
    ttfb_result = linux_measure_ttfb(http_port);
    
    /* Copy result to static buffer before stopping process */
    strncpy(result_buffer, ttfb_result, sizeof(result_buffer) - 1);
    result_buffer[sizeof(result_buffer) - 1] = '\0';
    
    stop_v2ray_process(pid);
#endif

    /* Clean up the temporary config file */
    unlink("ttfb_test_config.json");
    
    return result_buffer;
}