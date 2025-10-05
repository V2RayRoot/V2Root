#ifndef LIBV2ROOT_VMESS_H
#define LIBV2ROOT_VMESS_H

#include <stdio.h>
#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Parses a VMess configuration string and writes JSON config to file.
 */
EXPORT int parse_vmess_string(const char* vmess_str, FILE* fp, int http_port, int socks_port);

#ifdef __cplusplus
}
#endif

#endif /* LIBV2ROOT_VMESS_H */
