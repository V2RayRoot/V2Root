#ifndef LIBV2ROOT_SERVICE_H
#define LIBV2ROOT_SERVICE_H

#ifndef _WIN32

#include <sys/types.h>
#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Service management functions */
EXPORT int create_v2ray_service(const char* config_file, int http_port, int socks_port);
EXPORT int remove_v2ray_service(void);
EXPORT int start_v2ray_service(pid_t* pid);
EXPORT int stop_v2ray_service(void);
EXPORT int is_v2ray_service_running(void);

/* Proxy management */
EXPORT int set_system_proxy(int http_port, int socks_port);
EXPORT int unset_system_proxy(void);

#ifdef __cplusplus
}
#endif

#endif /* !_WIN32 */

#endif /* LIBV2ROOT_SERVICE_H */
