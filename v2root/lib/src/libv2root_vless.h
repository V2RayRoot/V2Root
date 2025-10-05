#ifndef LIBV2ROOT_VLESS_H
#define LIBV2ROOT_VLESS_H

#include <stdio.h>
#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a VLESS configuration string and writes JSON config to file.
 */
EXPORT int parse_vless_string(const char* vless_str, FILE* fp, int http_port, int socks_port);

#ifdef __cplusplus
}
#endif

#endif /* LIBV2ROOT_VLESS_H */
