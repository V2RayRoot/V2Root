#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <io.h>
    #define ACCESS _access
#else
    #include <sys/time.h>
    #include <netdb.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    #define ACCESS access
#endif

#include "libv2root_common.h"
#include "libv2root_core.h"
#include "libv2root_vless.h"
#include "libv2root_vmess.h"
#include "libv2root_shadowsocks.h"
#include "libv2root_manage.h"
#include "libv2root_utils.h"

/*
 * Loads the V2Ray configuration from config.json.
 *
 * Checks if the configuration file exists and logs the result.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   int: 0 on success, -1 if config.json is not found.
 *
 * Errors:
 *   Logs an error if the configuration file does not exist.
 */
EXPORT int load_v2ray_config() {
    if (ACCESS("config.json", F_OK) == -1) {
        log_message("config.json not found", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    log_message("Loaded config.json successfully", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Pings a server to measure latency.
 *
 * Establishes a TCP connection to the specified address and port, measuring the time taken.
 * Handles Windows and Linux platforms differently using platform-specific APIs.
 *
 * Parameters:
 *   address (const char*): The server address (IP or hostname) to ping.
 *   port (int): The port to connect to.
 *
 * Returns:
 *   int: Latency in milliseconds on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for socket creation, address resolution, or connection failures.
 */
EXPORT int ping_server(const char* address, int port) {
#ifdef _WIN32
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server;
    LARGE_INTEGER start, end, frequency;
    double elapsed_ms;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log_message("WSAStartup failed", __FILE__, __LINE__, WSAGetLastError(), NULL);
        return -1;
    }
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        log_message("Socket creation failed", __FILE__, __LINE__, WSAGetLastError(), NULL);
        WSACleanup();
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(address);
    server.sin_port = htons(port);

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        log_message("Ping connection failed", __FILE__, __LINE__, WSAGetLastError(), address);
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    QueryPerformanceCounter(&end);
    elapsed_ms = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, Latency: %.2fms", address, port, elapsed_ms);
    log_message("Ping successful", __FILE__, __LINE__, 0, extra_info);

    closesocket(sock);
    WSACleanup();
    return (int)elapsed_ms;
#else
    int sock;
    struct addrinfo hints, *res;
    struct timeval start, end;
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(address, port_str, &hints, &res) != 0) {
        log_message("Failed to resolve address", __FILE__, __LINE__, errno, address);
        return -1;
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        log_message("Socket creation failed", __FILE__, __LINE__, errno, NULL);
        freeaddrinfo(res);
        return -1;
    }

    gettimeofday(&start, NULL);
    if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
        log_message("Ping connection failed", __FILE__, __LINE__, errno, address);
        close(sock);
        freeaddrinfo(res);
        return -1;
    }
    gettimeofday(&end, NULL);

    int elapsed_ms = (int)(((end.tv_sec - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec) / 1000));

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, Latency: %dms", address, port, elapsed_ms);
    log_message("Ping successful", __FILE__, __LINE__, 0, extra_info);

    close(sock);
    freeaddrinfo(res);
    return elapsed_ms;
#endif
}