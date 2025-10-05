#ifndef LIBV2ROOT_UTILS_H
#define LIBV2ROOT_UTILS_H

#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Logging */
void log_message(const char* message, const char* file, int line, int error_code, const char* extra_info);

/* Validation */
int validate_address(const char* address);
int validate_port(const char* port_str);
int validate_uuid(const char* uuid);

/* String utilities */
void url_decode(char* dst, const char* src, size_t dst_size);
char* trim_whitespace(char* str);

/* New probe functions */
int send_http_probe(int sockfd, const char* host, const char* path, int* ttfb_ms);
double calculate_probe_score(int ttfb_ms, int tcp_ms, int success);

#ifdef __cplusplus
}
#endif

#endif /* LIBV2ROOT_UTILS_H */
