#ifndef LIBV2ROOT_MANAGE_H
#define LIBV2ROOT_MANAGE_H

#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Management and configuration functions.
 * These are declared here and implemented in libv2root_manage.c
 */
EXPORT char* measure_ttfb(const char* config_str, int http_port);

#ifdef __cplusplus
}
#endif

#endif /* LIBV2ROOT_MANAGE_H */
