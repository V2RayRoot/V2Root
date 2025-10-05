#ifndef LIBV2ROOT_WIN_H
#define LIBV2ROOT_WIN_H

#ifdef _WIN32

#include <windows.h>
#include "libv2root_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Process management */
int win_start_v2ray_process(const char* config_file, const char* v2ray_path, DWORD* pid);
int win_stop_v2ray_process(DWORD pid);

/* Registry operations for PID persistence */
void save_pid_to_registry(DWORD pid);
DWORD load_pid_from_registry(void);

/* Proxy settings */
int win_enable_system_proxy(int http_port, int socks_port);
int win_disable_system_proxy(void);

/* Connection testing */
int win_test_connection(int http_port, int* latency, HANDLE hProcess);
EXPORT char* win_measure_ttfb(int http_port);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* LIBV2ROOT_WIN_H */
