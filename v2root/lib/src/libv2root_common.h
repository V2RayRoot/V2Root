#ifndef LIBV2ROOT_COMMON_H
#define LIBV2ROOT_COMMON_H

#ifdef _WIN32
#include <windows.h>
#define EXPORT __declspec(dllexport)
#define PID_TYPE DWORD
#else
#include <sys/types.h>
#include <unistd.h>
#define EXPORT __attribute__((visibility("default")))
#define PID_TYPE pid_t
#endif

/* Default proxy ports */
#define DEFAULT_HTTP_PORT 2300
#define DEFAULT_SOCKS_PORT 2301

/* Error codes */
#define V2ROOT_SUCCESS 0
#define V2ROOT_ERROR -1
#define V2ROOT_ERROR_INVALID_INPUT -2
#define V2ROOT_ERROR_FILE_NOT_FOUND -3
#define V2ROOT_ERROR_CONFIG_MISSING -4
#define V2ROOT_ERROR_PROCESS_START -5
#define V2ROOT_ERROR_NETWORK -6

/* Log levels */
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR 3

/* Buffer sizes */
#define MAX_PATH_LENGTH 1024
#define MAX_ADDRESS_LENGTH 2048
#define MAX_PARAM_LENGTH 4096
#define MAX_UUID_LENGTH 128
#define MAX_PASSWORD_LENGTH 256

/* Protocol identifiers */
#define PROTOCOL_VLESS "vless://"
#define PROTOCOL_VMESS "vmess://"
#define PROTOCOL_SHADOWSOCKS "ss://"

/* Probe settings */
#define DEFAULT_DNS_TIMEOUT_MS 1000
#define DEFAULT_TCP_TIMEOUT_MS 2500
#define DEFAULT_TLS_TIMEOUT_MS 3000
#define DEFAULT_TRANSPORT_TIMEOUT_MS 3000
#define DEFAULT_TTFB_TIMEOUT_MS 5000
#define DEFAULT_PROBE_ATTEMPTS 3
#define MAX_CONCURRENT_PROBES 50

/* Probe endpoints */
#define PRIMARY_PROBE_URL "https://www.google.com/generate_204"
#define FALLBACK_PROBE_URL_1 "https://www.cloudflare.com/cdn-cgi/trace"
#define FALLBACK_PROBE_URL_2 "https://detectportal.firefox.com/success.txt"

/* Probe result structure */
typedef struct {
    int success;                    /* 0 = failed, 1 = success */
    int dns_ms;                     /* DNS resolution time */
    int tcp_connect_ms;             /* TCP connect time */
    int tls_handshake_ms;           /* TLS handshake time */
    int transport_handshake_ms;     /* Transport-specific handshake */
    int proxy_setup_ms;             /* Total proxy setup time */
    int app_connect_ms;             /* Proxied connection time */
    int ttfb_ms;                    /* Time to first byte */
    int total_ms;                   /* Total probe time */
    int attempts;                   /* Number of attempts */
    double score;                   /* Normalized score (0.0-1.0) */
    char error_type[64];            /* Error classification */
    char error_details[256];        /* Detailed error message */
} ProbeResult;

/* Error types */
#define PROBE_ERROR_NONE "none"
#define PROBE_ERROR_DNS "dns_failure"
#define PROBE_ERROR_TCP "tcp_timeout"
#define PROBE_ERROR_TLS "tls_error"
#define PROBE_ERROR_TRANSPORT "transport_error"
#define PROBE_ERROR_AUTH "auth_error"
#define PROBE_ERROR_UPSTREAM_BLOCKED "upstream_blocked"
#define PROBE_ERROR_TIMEOUT "timeout"
#define PROBE_ERROR_UNKNOWN "unknown"

#endif /* LIBV2ROOT_COMMON_H */
