#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <string.h>
#include <errno.h>
#include "libv2root_common.h"
#include "libv2root_linux.h"
#include "libv2root_service.h"
#include "libv2root_utils.h"

/*
 * Callback function for libcurl to handle response data.
 *
 * Discards the response data and returns the total size of the data received.
 *
 * Parameters:
 *   contents (void*): Pointer to the received data.
 *   size (size_t): Size of each data element.
 *   nmemb (size_t): Number of data elements.
 *   userp (void*): User-defined pointer (unused).
 *
 * Returns:
 *   size_t: Total size of the data (size * nmemb).
 *
 * Errors:
 *   None
 */
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

/*
 * Resets network proxy settings on Linux.
 *
 * Unsets system proxy environment variables and clears them from the parent shell.
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
EXPORT int linux_reset_network_proxy() {
    if (unset_system_proxy() != 0) {
        log_message("Failed to unset system proxy variables", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    int status = system("unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY socks_proxy SOCKS_PROXY");
    if (status == -1) {
        log_message("Failed to execute unset proxy command", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    if (WEXITSTATUS(status) != 0) {
        log_message("Unset proxy command failed", __FILE__, __LINE__, WEXITSTATUS(status), NULL);
        return -1;
    }
    log_message("Network proxy reset on Linux", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Enables system proxy settings on Linux for HTTP and SOCKS protocols.
 *
 * Sets environment variables for HTTP and SOCKS proxies in the parent shell.
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
EXPORT int linux_enable_system_proxy(int http_port, int socks_port) {
    if (set_system_proxy(http_port, socks_port) != 0) {
        log_message("Failed to set system proxy variables", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "export http_proxy=http://127.0.0.1:%d;"
             "export https_proxy=http://127.0.0.1:%d;"
             "export HTTP_PROXY=http://127.0.0.1:%d;"
             "export HTTPS_PROXY=http://127.0.0.1:%d;"
             "export socks_proxy=socks5://127.0.0.1:%d;"
             "export SOCKS_PROXY=socks5://127.0.0.1:%d",
             http_port, http_port, http_port, http_port, socks_port, socks_port);
    int status = system(cmd);
    if (status == -1) {
        log_message("Failed to execute set proxy command", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    if (WEXITSTATUS(status) != 0) {
        log_message("Set proxy command failed", __FILE__, __LINE__, WEXITSTATUS(status), NULL);
        return -1;
    }
    char extra_info[1024];
    snprintf(extra_info, sizeof(extra_info), "HTTP Proxy: http://127.0.0.1:%d, SOCKS Proxy: socks5://127.0.0.1:%d", http_port, socks_port);
    log_message("System proxy enabled on Linux", __FILE__, __LINE__, 0, extra_info);
    return 0;
}

/*
 * Disables system proxy settings on Linux.
 *
 * Unsets proxy environment variables and clears them from the parent shell.
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
EXPORT int linux_disable_system_proxy(void) {
    if (unset_system_proxy() != 0) {
        log_message("Failed to unset system proxy variables", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    int status = system("unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY socks_proxy SOCKS_PROXY");
    if (status == -1) {
        log_message("Failed to execute unset proxy command for disable", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    if (WEXITSTATUS(status) != 0) {
        log_message("Disable proxy command failed", __FILE__, __LINE__, WEXITSTATUS(status), NULL);
        return -1;
    }
    log_message("System proxy disabled on Linux", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Starts a V2Ray process on Linux using the specified configuration file.
 *
 * Creates and starts a V2Ray service, ensuring no existing service is running.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   pid (pid_t*): Pointer to store the process ID of the started V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -1 if the config file is missing or the service fails to start.
 *
 * Errors:
 *   Logs errors if the config file is not found, a service is already running, or service creation/start fails.
 */
EXPORT int linux_start_v2ray_process(const char* config_file, pid_t* pid) {
    if (access(config_file, F_OK) == -1) {
        log_message("Config file not found", __FILE__, __LINE__, errno, config_file);
        return -1;
    }

    if (is_v2ray_service_running()) {
        log_message("v2ray service already running", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    if (create_v2ray_service(config_file, 2300, 2301) != 0) {
        log_message("Failed to create v2ray service", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    if (start_v2ray_service(pid) != 0) {
        log_message("Failed to start v2ray service", __FILE__, __LINE__, 0, NULL);
        remove_v2ray_service();
        return -1;
    }

    char pid_info[256];
    snprintf(pid_info, sizeof(pid_info), "V2Ray process started with PID: %ld", (long)*pid);
    log_message(pid_info, __FILE__, __LINE__, 0, NULL);

    return 0;
}

/*
 * Stops a V2Ray process on Linux.
 *
 * Stops the V2Ray service, removes it, and resets network proxy settings.
 *
 * Parameters:
 *   pid (pid_t): The process ID of the V2Ray process to stop.
 *
 * Returns:
 *   int: 0 on success, -1 if stopping or cleaning up the service fails.
 *
 * Errors:
 *   Logs errors if stopping the service, removing the service, or resetting the proxy fails.
 */
EXPORT int linux_stop_v2ray_process(pid_t pid) {
    if (stop_v2ray_service() != 0) {
        log_message("Failed to stop v2ray service", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    if (remove_v2ray_service() != 0) {
        log_message("Failed to clean up v2ray service", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    if (linux_reset_network_proxy() != 0) {
        log_message("Failed to reset network proxy after stopping V2Ray", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    return 0;
}

/*
 * Tests the connectivity and latency of a V2Ray configuration on Linux.
 *
 * Uses libcurl to send an HTTP request through the specified proxy port and measures the response time.
 *
 * Parameters:
 *   http_port (int): The HTTP proxy port to test.
 *   socks_port (int): The SOCKS proxy port (unused in this implementation).
 *   latency (int*): Pointer to store the measured latency in milliseconds.
 *   pid (pid_t): The process ID of the V2Ray process (unused in this implementation).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if curl initialization or the HTTP request fails.
 */
EXPORT int linux_test_connection(int http_port, int socks_port, int* latency, pid_t pid) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        log_message("Failed to initialize curl", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    char proxy[256];
    snprintf(proxy, sizeof(proxy), "http://127.0.0.1:%d", http_port);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.myip.com");
    curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    CURLcode res = curl_easy_perform(curl);
    gettimeofday(&end, NULL);

    if (res != CURLE_OK) {
        log_message("Connection test failed", __FILE__, __LINE__, res, curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }

    *latency = (int)(((end.tv_sec - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec) / 1000));
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Latency: %dms", *latency);
    log_message("Connection test successful", __FILE__, __LINE__, 0, extra_info);

    curl_easy_cleanup(curl);
    return 0;
}