#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <curl/curl.h>
#include "libv2root_linux.h"
#include "libv2root_utils.h"

/*
 * Starts a V2Ray process using fork/exec.
 * 
 * NOTE: On Linux, this function ALWAYS uses the system-installed 'v2ray' command
 * found in PATH. The config_file parameter is used, but the v2ray executable
 * must be installed via package manager (apt, dnf, pacman, etc.).
 * 
 * This ensures consistent behavior and proper system integration on Linux platforms.
 */
int linux_start_v2ray_process(const char* config_file, pid_t* pid) {
    if (!config_file || !pid) {
        log_message("Invalid arguments to linux_start_v2ray_process", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    *pid = fork();
    
    if (*pid == -1) {
        log_message("Failed to fork process", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    
    if (*pid == 0) {
        /* Child process */
        /* IMPORTANT: Always use "v2ray" command from system PATH on Linux */
        /* This ensures we use the package manager-installed V2Ray */
        char* args[] = {"v2ray", "run", "-c", (char*)config_file, NULL};
        execvp(args[0], args);  /* execvp searches PATH for "v2ray" */
        
        /* If execvp returns, it failed */
        log_message("Failed to execute V2Ray - ensure V2Ray is installed via package manager", __FILE__, __LINE__, errno, NULL);
        _exit(1);
    }
    
    /* Parent process */
    usleep(500000); /* Wait 500ms for process to start */
    
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "V2Ray process started with PID: %d using system-installed v2ray", *pid);
    log_message("Linux V2Ray process started", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Stops a V2Ray process by sending SIGTERM.
 */
int linux_stop_v2ray_process(pid_t pid) {
    if (pid <= 0) {
        log_message("Invalid PID for stop", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    if (kill(pid, SIGTERM) == -1) {
        if (errno == ESRCH) {
            log_message("Process not found", __FILE__, __LINE__, errno, NULL);
            return 0;
        }
        log_message("Failed to stop V2Ray process", __FILE__, __LINE__, errno, NULL);
        return -1;
    }
    
    /* Wait for process to terminate */
    int status;
    for (int i = 0; i < 10; i++) {
        if (waitpid(pid, &status, WNOHANG) == pid) {
            log_message("V2Ray process terminated", __FILE__, __LINE__, 0, NULL);
            return 0;
        }
        usleep(100000); /* Wait 100ms */
    }
    
    /* Force kill if still running */
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    
    log_message("V2Ray process force killed", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Enables system proxy by setting environment variables.
 */
int linux_enable_system_proxy(int http_port, int socks_port) {
    char http_proxy[64], socks_proxy[64];
    
    snprintf(http_proxy, sizeof(http_proxy), "http://127.0.0.1:%d", http_port);
    snprintf(socks_proxy, sizeof(socks_proxy), "socks5://127.0.0.1:%d", socks_port);
    
    setenv("http_proxy", http_proxy, 1);
    setenv("https_proxy", http_proxy, 1);
    setenv("HTTP_PROXY", http_proxy, 1);
    setenv("HTTPS_PROXY", http_proxy, 1);
    setenv("socks_proxy", socks_proxy, 1);
    setenv("SOCKS_PROXY", socks_proxy, 1);
    
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "HTTP: %s, SOCKS: %s", http_proxy, socks_proxy);
    log_message("Linux system proxy enabled", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Disables system proxy by unsetting environment variables.
 */
int linux_disable_system_proxy(void) {
    unsetenv("http_proxy");
    unsetenv("https_proxy");
    unsetenv("HTTP_PROXY");
    unsetenv("HTTPS_PROXY");
    unsetenv("socks_proxy");
    unsetenv("SOCKS_PROXY");
    
    log_message("Linux system proxy disabled", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Resets network proxy settings.
 */
int linux_reset_network_proxy(void) {
    return linux_disable_system_proxy();
}

/* Callback for curl - we don't need the data, just need to trigger the request */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;  /* Return size to indicate success */
}

/*
 * Tests connection latency through the proxy by making a real HTTP request.
 * This measures actual end-to-end latency through the V2Ray node.
 */
int linux_test_connection(int http_port, int socks_port, int* latency, pid_t pid) {
    if (!latency) {
        log_message("Null latency pointer", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    CURL *curl;
    CURLcode res;
    struct timeval start, end;
    
    curl = curl_easy_init();
    if (!curl) {
        log_message("Failed to initialize curl", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    // Configure proxy
    char proxy_str[128];
    snprintf(proxy_str, sizeof(proxy_str), "http://127.0.0.1:%d", http_port);
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com/generate_204");
    curl_easy_setopt(curl, CURLOPT_PROXY, proxy_str);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  /* Skip SSL verification for test */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "V2Root-Test/1.0");
    
    // Start timing
    gettimeofday(&start, NULL);
    
    // Perform the request
    res = curl_easy_perform(curl);
    
    // Stop timing
    gettimeofday(&end, NULL);
    
    if (res != CURLE_OK) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        log_message("Failed to perform HTTP request via proxy", __FILE__, __LINE__, 0, err_msg);
        curl_easy_cleanup(curl);
        return -1;
    }
    
    // Calculate latency
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed_ms = (seconds * 1000.0) + (microseconds / 1000.0);
    
    *latency = (int)(elapsed_ms + 0.5);
    
    // Ensure minimum 1ms
    if (*latency < 1) {
        *latency = 1;
    }
    
    // Get detailed timing info from curl
    double total_time = 0, namelookup_time = 0, connect_time = 0, pretransfer_time = 0;
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);
    curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
    curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &connect_time);
    curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &pretransfer_time);
    
    curl_easy_cleanup(curl);
    
    char extra_info[512];
    snprintf(extra_info, sizeof(extra_info), 
             "Real connection latency: %d ms (DNS: %.0fms, Connect: %.0fms, Total: %.0fms)", 
             *latency, namelookup_time * 1000, connect_time * 1000, total_time * 1000);
    log_message("Connection test successful via proxy", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Performs a single HTTP request through the V2Ray proxy and measures TTFB.
 * Returns JSON with platform, success, ttfb_ms, http_status, and error_message.
 */
EXPORT char* linux_measure_ttfb(int http_port) {
    static char result[1024];
    CURL *curl;
    CURLcode res;
    long http_code = 0;
    double total_time = 0;
    
    curl = curl_easy_init();
    if (!curl) {
        snprintf(result, sizeof(result),
                 "{\"platform\": \"linux\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to initialize curl\"}");
        return result;
    }
    
    // Configure proxy
    char proxy_str[128];
    snprintf(proxy_str, sizeof(proxy_str), "http://127.0.0.1:%d", http_port);
    
    // Set curl options with shorter timeouts
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com/generate_204");
    curl_easy_setopt(curl, CURLOPT_PROXY, proxy_str);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);  /* Reduced from 10 to 5 seconds */
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);  /* Reduced from 10 to 3 seconds */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "V2Root-TTFBTest/1.0");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);  /* Prevent signals from interrupting */
    
    // Perform the request
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        const char* error_str = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        snprintf(result, sizeof(result),
                 "{\"platform\": \"linux\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"%s\"}", 
                 error_str);
        return result;
    }
    
    // Get HTTP status code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    // Get TTFB - curl provides time to first byte via CURLINFO_STARTTRANSFER_TIME
    curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &total_time);
    int ttfb_ms = (int)(total_time * 1000 + 0.5);
    
    // Ensure minimum value
    if (ttfb_ms < 1) {
        ttfb_ms = 1;
    }
    
    // Cleanup
    curl_easy_cleanup(curl);
    
    // Return formatted result
    snprintf(result, sizeof(result),
             "{\"platform\": \"linux\", \"success\": true, \"ttfb_ms\": %d, \"http_status\": %ld, \"error_message\": null}", 
             ttfb_ms, http_code);
    
    return result;
}

#endif /* !_WIN32 */
   