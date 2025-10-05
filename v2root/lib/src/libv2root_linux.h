#ifndef LIBV2ROOT_LINUX_H
#define LIBV2ROOT_LINUX_H

#ifndef _WIN32

#include <sys/types.h>
#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Process management */
int linux_start_v2ray_process(const char* config_file, pid_t* pid);
int linux_stop_v2ray_process(pid_t pid);

/* Proxy settings */
int linux_enable_system_proxy(int http_port, int socks_port);
int linux_disable_system_proxy(void);
int linux_reset_network_proxy(void);

/* Connection testing */
int linux_test_connection(int http_port, int socks_port, int* latency, pid_t pid);
EXPORT char* linux_measure_ttfb(int http_port);

#ifdef __cplusplus
}
#endif

#endif /* !_WIN32 */

#endif /* LIBV2ROOT_LINUX_H */
