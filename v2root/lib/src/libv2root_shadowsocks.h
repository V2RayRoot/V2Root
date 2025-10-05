#ifndef LIBV2ROOT_SHADOWSOCKS_H
#define LIBV2ROOT_SHADOWSOCKS_H

#include <stdio.h>
#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a Shadowsocks configuration string and writes JSON config to file.
 */
EXPORT int parse_shadowsocks_string(const char* ss_str, FILE* fp, int http_port, int socks_port);

#ifdef __cplusplus
}
#endif

#endif /* LIBV2ROOT_SHADOWSOCKS_H */
