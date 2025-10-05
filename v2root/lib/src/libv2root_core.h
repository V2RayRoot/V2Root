#ifndef LIBV2ROOT_CORE_H
#define LIBV2ROOT_CORE_H

#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Core initialization and management */
EXPORT int init_v2ray(const char* config_file, const char* v2ray_path);
EXPORT int start_v2ray(int http_port, int socks_port);
EXPORT int start_v2ray_with_pid(int http_port, int socks_port, PID_TYPE* pid);
EXPORT int stop_v2ray(void);
EXPORT int reset_network_proxy(void);

/* Configuration parsing */
EXPORT int parse_config_string(const char* config_str, int http_port, int socks_port);

/* Connection testing */
EXPORT int test_config_connection(const char* config_str, int* latency, int http_port, int socks_port);

/* Ping server */
EXPORT int ping_server(const char* address, int port);

/* Advanced connection testing with end-to-end probe */
EXPORT int probe_config_full(const char* config_str, ProbeResult* result, int http_port, int socks_port, int attempts);
EXPORT int probe_config_quick(const char* config_str, ProbeResult* result, int http_port, int socks_port);

#ifdef __cplusplus
}
#endif

#endif /* LIBV2ROOT_CORE_H */
