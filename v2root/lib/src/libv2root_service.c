#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <jansson.h>
#include "libv2root_common.h"
#include "libv2root_service.h"
#include "libv2root_utils.h"

#define PID_FILE "/tmp/v2root.pid"
#define PROXY_ENV_FILE "/tmp/v2root_proxy_env.sh"
#define SERVICE_JSON_FILE "~/.v2root/v2ray_service.json"

static char service_json_path[1024];

/*
 * Expands the home directory in a path (replacing ~ with $HOME).
 *
 * Parameters:
 *   path (const char*): Input path that may contain ~.
 *   expanded (char*): Buffer to store the expanded path.
 *   max_len (size_t): Maximum length of the expanded buffer.
 *
 * Returns:
 *   int: 0 on success, -1 if home directory is invalid or path is too long.
 */
static int expand_home_path(const char* path, char* expanded, size_t max_len) {
    if (path[0] != '~') {
        if (strlen(path) >= max_len) return -1;
        strcpy(expanded, path);
        return 0;
    }

    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : "/tmp";
    }
    size_t home_len = strlen(home);
    size_t path_len = strlen(path) - 1; // Skip ~
    if (home_len + path_len >= max_len) return -1;

    snprintf(expanded, max_len, "%s%s", home, path + 1);
    return 0;
}

/*
 * Creates directories recursively, similar to mkdir -p.
 *
 * Parameters:
 *   path (const char*): The directory path to create.
 *   mode (mode_t): Permissions for the new directories.
 *
 * Returns:
 *   int: 0 on success, -1 if directory creation fails (except when it already exists).
 */
static int mkdir_p(const char* path, mode_t mode) {
    char tmp[1024];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) == -1 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, mode) == -1 && errno != EEXIST) return -1;
    return 0;
}

/*
 * Initializes the path for the service JSON file.
 *
 * Constructs the path in the user's home directory (~/.v2root/).
 *
 * Returns:
 *   int: 0 on success, -1 if path construction fails.
 */
static int init_service_json_path() {
    if (expand_home_path(SERVICE_JSON_FILE, service_json_path, sizeof(service_json_path)) != 0) {
        log_message("Failed to expand service JSON path", __FILE__, __LINE__, 0, SERVICE_JSON_FILE);
        return -1;
    }

    char* dir = strdup(service_json_path);
    if (!dir) {
        log_message("Failed to allocate memory for directory path", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    char* last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (mkdir_p(dir, 0700) == -1) {
            log_message("Failed to create directory for service JSON", __FILE__, __LINE__, errno, dir);
            free(dir);
            return -1;
        }
    }
    free(dir);
    return 0;
}

/*
 * Checks if a process with the given PID is running.
 *
 * Sends a signal 0 to the process to check its existence.
 *
 * Parameters:
 *   pid (pid_t): The process ID to check.
 *
 * Returns:
 *   int: 1 if the process is running, 0 otherwise.
 */
static int is_pid_running(pid_t pid) {
    return (pid > 0 && kill(pid, 0) == 0) ? 1 : 0;
}

/*
 * Reads the process ID from the PID file.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   pid_t: The process ID, or 0 if the file cannot be read or is invalid.
 */
__attribute__((used)) static pid_t read_pid_file() {
    FILE* fp = fopen(PID_FILE, "r");
    if (!fp) return 0;
    pid_t pid;
    if (fscanf(fp, "%d", &pid) != 1) pid = 0;
    fclose(fp);
    return pid;
}

/*
 * Writes a process ID to the PID file.
 *
 * Parameters:
 *   pid (pid_t): The process ID to write.
 *
 * Returns:
 *   None
 */
static void write_pid_file(pid_t pid) {
    FILE* fp = fopen(PID_FILE, "w");
    if (fp) {
        fprintf(fp, "%d", pid);
        fclose(fp);
    } else {
        log_message("Failed to write PID file", __FILE__, __LINE__, errno, PID_FILE);
    }
}

/*
 * Removes the PID file.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None
 */
static void remove_pid_file() {
    if (unlink(PID_FILE) == -1 && errno != ENOENT) {
        log_message("Failed to remove PID file", __FILE__, __LINE__, errno, PID_FILE);
    }
}

/*
 * Saves service information to a JSON file.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   http_port (int): HTTP proxy port.
 *   socks_port (int): SOCKS proxy port.
 *   pid (pid_t): Process ID of the V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
static int save_service_json(const char* config_file, int http_port, int socks_port, pid_t pid) {
    json_t* root = json_object();
    if (!root) {
        log_message("Failed to create JSON object", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    json_object_set_new(root, "config_file", json_string(config_file));
    json_object_set_new(root, "http_port", json_integer(http_port));
    json_object_set_new(root, "socks_port", json_integer(socks_port));
    json_object_set_new(root, "pid", json_integer(pid));

    if (json_dump_file(root, service_json_path, JSON_INDENT(4)) != 0) {
        log_message("Failed to write service JSON file", __FILE__, __LINE__, errno, service_json_path);
        json_decref(root);
        return -1;
    }

    json_decref(root);
    log_message("Service JSON saved successfully", __FILE__, __LINE__, 0, service_json_path);
    return 0;
}

/*
 * Loads service information from the JSON file.
 *
 * Parameters:
 *   config_file (char*): Buffer to store the config file path.
 *   config_file_len (size_t): Size of the config file buffer.
 *   http_port (int*): Pointer to store the HTTP port.
 *   socks_port (int*): Pointer to store the SOCKS port.
 *   pid (pid_t*): Pointer to store the process ID.
 *
 * Returns:
 *   int: 0 on success, -1 on failure or if file doesn't exist.
 */
static int load_service_json(char* config_file, size_t config_file_len, int* http_port, int* socks_port, pid_t* pid) {
    json_error_t error;
    json_t* root = json_load_file(service_json_path, 0, &error);
    if (!root) {
        log_message("Failed to load service JSON file", __FILE__, __LINE__, 0, error.text);
        return -1;
    }

    const char* config = json_string_value(json_object_get(root, "config_file"));
    if (config && strlen(config) < config_file_len) {
        strcpy(config_file, config);
    } else {
        log_message("Invalid or missing config_file in JSON", __FILE__, __LINE__, 0, NULL);
        json_decref(root);
        return -1;
    }

    *http_port = json_integer_value(json_object_get(root, "http_port"));
    *socks_port = json_integer_value(json_object_get(root, "socks_port"));
    *pid = json_integer_value(json_object_get(root, "pid"));

    json_decref(root);
    return 0;
}

/*
 * Removes the service JSON file.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
static int remove_service_json() {
    if (unlink(service_json_path) == -1 && errno != ENOENT) {
        log_message("Failed to remove service JSON file", __FILE__, __LINE__, errno, service_json_path);
        return -1;
    }
    log_message("Service JSON file removed", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Sets system proxy environment variables for HTTP and SOCKS protocols.
 *
 * Parameters:
 *   http_port (int): The port for the HTTP proxy.
 *   socks_port (int): The port for the SOCKS proxy.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int set_system_proxy(int http_port, int socks_port) {
    char http_proxy[64], socks_proxy[64];
    snprintf(http_proxy, sizeof(http_proxy), "http://127.0.0.1:%d", http_port);
    snprintf(socks_proxy, sizeof(socks_proxy), "socks5://127.0.0.1:%d", socks_port);

    if (setenv("http_proxy", http_proxy, 1) != 0 ||
        setenv("https_proxy", http_proxy, 1) != 0 ||
        setenv("HTTP_PROXY", http_proxy, 1) != 0 ||
        setenv("HTTPS_PROXY", http_proxy, 1) != 0 ||
        setenv("socks_proxy", socks_proxy, 1) != 0 ||
        setenv("SOCKS_PROXY", socks_proxy, 1) != 0) {
        log_message("Failed to set system proxy environment variables", __FILE__, __LINE__, errno, NULL);
        return -1;
    }

    FILE* fp = fopen(PROXY_ENV_FILE, "w");
    if (!fp) {
        log_message("Failed to create proxy env file", __FILE__, __LINE__, errno, PROXY_ENV_FILE);
        return -1;
    }

    fprintf(fp, "export http_proxy=%s\n", http_proxy);
    fprintf(fp, "export https_proxy=%s\n", http_proxy);
    fprintf(fp, "export HTTP_PROXY=%s\n", http_proxy);
    fprintf(fp, "export HTTPS_PROXY=%s\n", http_proxy);
    fprintf(fp, "export socks_proxy=%s\n", socks_proxy);
    fprintf(fp, "export SOCKS_PROXY=%s\n", socks_proxy);
    fclose(fp);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "export http_proxy=%s;"
             "export https_proxy=%s;"
             "export HTTP_PROXY=%s;"
             "export HTTPS_PROXY=%s;"
             "export socks_proxy=%s;"
             "export SOCKS_PROXY=%s",
             http_proxy, http_proxy, http_proxy, http_proxy, socks_proxy, socks_proxy);
    int status = system(cmd);
    if (status == -1) {
        log_message("Failed to execute set proxy command", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    if (WEXITSTATUS(status) != 0) {
        log_message("Set proxy command failed", __FILE__, __LINE__, WEXITSTATUS(status), NULL);
        return -1;
    }

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Set system proxy: HTTP=%s, SOCKS=%s", http_proxy, socks_proxy);
    log_message("System proxy set successfully", __FILE__, __LINE__, 0, extra_info);
    return 0;
}

/*
 * Unsets system proxy environment variables.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int unset_system_proxy() {
    unsetenv("http_proxy");
    unsetenv("https_proxy");
    unsetenv("socks_proxy");
    unsetenv("HTTP_PROXY");
    unsetenv("HTTPS_PROXY");
    unsetenv("SOCKS_PROXY");

    FILE* fp = fopen(PROXY_ENV_FILE, "w");
    if (!fp) {
        log_message("Failed to create proxy env file", __FILE__, __LINE__, errno, PROXY_ENV_FILE);
        return -1;
    }

    fprintf(fp, "unset http_proxy\n");
    fprintf(fp, "unset https_proxy\n");
    fprintf(fp, "unset HTTP_PROXY\n");
    fprintf(fp, "unset HTTPS_PROXY\n");
    fprintf(fp, "unset socks_proxy\n");
    fprintf(fp, "unset SOCKS_PROXY\n");
    fclose(fp);

    int status = system("unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY socks_proxy SOCKS_PROXY");
    if (status == -1) {
        log_message("Failed to execute unset proxy command", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    if (WEXITSTATUS(status) != 0) {
        log_message("Unset proxy command failed", __FILE__, __LINE__, WEXITSTATUS(status), NULL);
        return -1;
    }

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "System proxy cleared");
    log_message("System proxy environment variables cleared", __FILE__, __LINE__, 0, extra_info);
    return 0;
}

/*
 * Creates a V2Ray service configuration and saves it to JSON.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   http_port (int): The HTTP proxy port.
 *   socks_port (int): The SOCKS proxy port.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int create_v2ray_service(const char* config_file, int http_port, int socks_port) {
    if (init_service_json_path() != 0) return -1;

    // Save service info to JSON (no PID yet)
    if (save_service_json(config_file, http_port, socks_port, 0) != 0) {
        return -1;
    }

    log_message("V2Ray service configuration created successfully", __FILE__, __LINE__, 0, service_json_path);
    return 0;
}

/*
 * Removes the V2Ray service configuration.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int remove_v2ray_service() {
    if (init_service_json_path() != 0) return -1;

    if (remove_service_json() != 0) {
        return -1;
    }

    log_message("V2Ray service configuration removed successfully", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Starts the V2Ray service by forking a process.
 *
 * Parameters:
 *   pid (pid_t*): Pointer to store the process ID of the started V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int start_v2ray_service(pid_t* pid) {
    if (init_service_json_path() != 0) return -1;

    char config_file[1024];
    int http_port, socks_port;
    pid_t existing_pid;
    if (load_service_json(config_file, sizeof(config_file), &http_port, &socks_port, &existing_pid) != 0) {
        log_message("Failed to load service JSON for starting service", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    if (existing_pid > 0 && is_pid_running(existing_pid)) {
        char extra_info[256];
        snprintf(extra_info, sizeof(extra_info), "V2Ray already running with PID: %d", existing_pid);
        log_message("V2Ray service already running", __FILE__, __LINE__, 0, extra_info);
        *pid = existing_pid;
        return 0;
    }

    *pid = fork();
    if (*pid == -1) {
        log_message("Failed to fork process for V2Ray", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    if (*pid == 0) {
        char http_proxy[64], socks_proxy[64];
        snprintf(http_proxy, sizeof(http_proxy), "http://127.0.0.1:%d", http_port);
        snprintf(socks_proxy, sizeof(socks_proxy), "socks5://127.0.0.1:%d", socks_port);
        setenv("http_proxy", http_proxy, 1);
        setenv("https_proxy", http_proxy, 1);
        setenv("HTTP_PROXY", http_proxy, 1);
        setenv("HTTPS_PROXY", http_proxy, 1);
        setenv("socks_proxy", socks_proxy, 1);
        setenv("SOCKS_PROXY", socks_proxy, 1);

        char* args[] = {"/usr/bin/v2ray", "run", "-c", config_file, NULL};
        execv(args[0], args);
        log_message("Failed to execute V2Ray", __FILE__, __LINE__, errno, NULL);
        _exit(1);
    }

    write_pid_file(*pid);
    if (save_service_json(config_file, http_port, socks_port, *pid) != 0) {
        kill(*pid, SIGTERM);
        return -1;
    }

    if (set_system_proxy(http_port, socks_port) != 0) {
        log_message("Failed to set system proxy after starting V2Ray", __FILE__, __LINE__, 0, NULL);
        kill(*pid, SIGTERM);
        return -1;
    }

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "V2Ray started with PID: %d", *pid);
    log_message("V2Ray service started", __FILE__, __LINE__, 0, extra_info);
    return 0;
}

/*
 * Stops the V2Ray service.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 */
EXPORT int stop_v2ray_service() {
    if (init_service_json_path() != 0) return -1;

    char config_file[1024];
    int http_port, socks_port;
    pid_t pid;
    if (load_service_json(config_file, sizeof(config_file), &http_port, &socks_port, &pid) != 0) {
        log_message("Failed to load service JSON for stopping service", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    if (pid > 0 && is_pid_running(pid)) {
        if (kill(pid, SIGTERM) == 0) {
            log_message("V2Ray process stopped", __FILE__, __LINE__, 0, NULL);
        } else {
            log_message("Failed to stop V2Ray process", __FILE__, __LINE__, errno, NULL);
            return -1;
        }
    }

    remove_pid_file();
    if (save_service_json(config_file, http_port, socks_port, 0) != 0) {
        return -1;
    }

    if (unset_system_proxy() != 0) {
        log_message("Failed to unset system proxy after stopping V2Ray", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    log_message("V2Ray service stopped", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Checks if the V2Ray service is running.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 1 if the service is running, 0 otherwise.
 */
EXPORT int is_v2ray_service_running() {
    if (init_service_json_path() != 0) return 0;

    char config_file[1024];
    int http_port, socks_port;
    pid_t pid;
    if (load_service_json(config_file, sizeof(config_file), &http_port, &socks_port, &pid) != 0) {
        return 0;
    }

    if (pid > 0 && is_pid_running(pid)) {
        char extra_info[256];
        snprintf(extra_info, sizeof(extra_info), "V2Ray process detected with PID: %d", pid);
        log_message("V2Ray process detected", __FILE__, __LINE__, 0, extra_info);
        return 1;
    }

    remove_pid_file();
    log_message("No V2Ray process found", __FILE__, __LINE__, 0, NULL);
    return 0;
}