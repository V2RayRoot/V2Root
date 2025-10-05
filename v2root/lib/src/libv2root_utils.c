#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "libv2root_utils.h"

#define LOG_FILE "v2root.log"

/*
 * Logs a message with timestamp, file location, and optional error information.
 * Writes to v2root.log file ONLY - no terminal output.
 */
void log_message(const char* message, const char* file, int line, int error_code, const char* extra_info) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    /* Open log file in append mode */
    FILE* log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) {
        /* If we can't open the log file, silently fail - no terminal output */
        return;
    }
    
    fprintf(log_fp, "[%s] %s:%d - %s", timestamp, file, line, message);
    
    if (error_code != 0) {
        fprintf(log_fp, " (Error code: %d)", error_code);
    }
    
    if (extra_info != NULL && extra_info[0] != '\0') {
        fprintf(log_fp, " - %s", extra_info);
    }
    
    fprintf(log_fp, "\n");
    fflush(log_fp);
    fclose(log_fp);
}

/*
 * Validates an IP address or domain name.
 */
int validate_address(const char* address) {
    if (!address || address[0] == '\0') {
        return 0;
    }
    
    /* Check for IPv6 (contains colons) */
    if (strchr(address, ':') != NULL) {
        struct sockaddr_in6 sa;
        return inet_pton(AF_INET6, address, &(sa.sin6_addr)) == 1;
    }
    
    /* Check for IPv4 */
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, address, &(sa.sin_addr)) == 1) {
        return 1;
    }
    
    /* Validate domain name */
    size_t len = strlen(address);
    if (len > 253) return 0;
    
    for (size_t i = 0; i < len; i++) {
        char c = address[i];
        if (!isalnum(c) && c != '.' && c != '-' && c != '_') {
            return 0;
        }
    }
    
    return 1;
}

/*
 * Validates a port number string.
 */
int validate_port(const char* port_str) {
    if (!port_str || port_str[0] == '\0') {
        return 0;
    }
    
    for (size_t i = 0; port_str[i] != '\0'; i++) {
        if (!isdigit(port_str[i])) {
            return 0;
        }
    }
    
    int port = atoi(port_str);
    return (port > 0 && port <= 65535);
}

/*
 * Validates a UUID string format.
 */
int validate_uuid(const char* uuid) {
    if (!uuid || strlen(uuid) != 36) {
        return 0;
    }
    
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (uuid[i] != '-') return 0;
        } else {
            if (!isxdigit(uuid[i])) return 0;
        }
    }
    
    return 1;
}

/*
 * URL decodes a string in place.
 */
void url_decode(char* dst, const char* src, size_t dst_size) {
    size_t i = 0, j = 0;
    
    while (src[i] != '\0' && j < dst_size - 1) {
        if (src[i] == '%' && isxdigit(src[i + 1]) && isxdigit(src[i + 2])) {
            char hex[3] = {src[i + 1], src[i + 2], '\0'};
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    
    dst[j] = '\0';
}

/*
 * Trims leading and trailing whitespace from a string.
 */
char* trim_whitespace(char* str) {
    if (!str) return NULL;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == '\0') return str;
    
    /* Trim trailing space */
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = '\0';
    
    return str;
}

/*
 * Sends HTTP GET request and measures time to first byte.
 * Returns 0 on success with ttfb_ms filled, -1 on failure.
 */
int send_http_probe(int sockfd, const char* host, const char* path, int* ttfb_ms) {
    if (!host || !path || !ttfb_ms) return -1;
    
    char request[1024];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: V2Root-Probe/1.0\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, host);
    
#ifdef _WIN32
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    if (send(sockfd, request, strlen(request), 0) == SOCKET_ERROR) {
        return -1;
    }
    
    char response[512];
    int received = recv(sockfd, response, sizeof(response) - 1, 0);
    
    QueryPerformanceCounter(&end);
    
    if (received > 0) {
        response[received] = '\0';
        double elapsed = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / (double)freq.QuadPart;
        *ttfb_ms = (int)(elapsed + 0.5);
        if (*ttfb_ms < 1) *ttfb_ms = 1;
        
        /* Basic HTTP response validation */
        if (strstr(response, "HTTP/1.") && (strstr(response, " 204 ") || strstr(response, " 200 ") || strstr(response, " 301 "))) {
            return 0;
        }
        return -1; /* Invalid response */
    }
    return -1;
#else
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    if (send(sockfd, request, strlen(request), 0) < 0) {
        return -1;
    }
    
    char response[512];
    ssize_t received = recv(sockfd, response, sizeof(response) - 1, 0);
    
    gettimeofday(&end, NULL);
    
    if (received > 0) {
        response[received] = '\0';
        long seconds = end.tv_sec - start.tv_sec;
        long microseconds = end.tv_usec - start.tv_usec;
        double elapsed = (seconds * 1000.0) + (microseconds / 1000.0);
        *ttfb_ms = (int)(elapsed + 0.5);
        if (*ttfb_ms < 1) *ttfb_ms = 1;
        
        /* Basic HTTP response validation */
        if (strstr(response, "HTTP/1.") && (strstr(response, " 204 ") || strstr(response, " 200 ") || strstr(response, " 301 "))) {
            return 0;
        }
        return -1; /* Invalid response */
    }
    return -1;
#endif
}

/*
 * Calculates normalized score from latency metrics.
 * Returns score between 0.0 (worst) and 1.0 (best).
 */
double calculate_probe_score(int ttfb_ms, int tcp_ms, int success) {
    if (!success) return 0.0;
    
    /* Utility function: u = 1 / (1 + ms/100) */
    double u_ttfb = 1.0 / (1.0 + (ttfb_ms / 100.0));
    double u_tcp = 1.0 / (1.0 + (tcp_ms / 100.0));
    
    /* Weighted score: 70% app-level, 25% TCP, 5% success bonus */
    double score = (0.70 * u_ttfb) + (0.25 * u_tcp) + 0.05;
    
    /* Clamp to [0, 1] */
    if (score < 0.0) score = 0.0;
    if (score > 1.0) score = 1.0;
    
    return score;
}
