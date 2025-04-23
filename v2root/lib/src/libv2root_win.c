#include <windows.h>
#include <winhttp.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include "libv2root_common.h"
#include "libv2root_manage.h"
#include "libv2root_utils.h"
#include "libv2root_win.h"

/*
 * Saves the V2Ray process ID to the Windows registry under HKEY_CURRENT_USER\Software\V2Root.
 *
 * Parameters:
 *   pid (PID_TYPE): The process ID to save.
 *
 * Errors:
 *   Logs errors if registry operations fail.
 */
__declspec(dllexport) void save_pid_to_registry(PID_TYPE pid) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\V2Root", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        if (RegCreateKeyA(HKEY_CURRENT_USER, "Software\\V2Root", &hKey) != ERROR_SUCCESS) {
            log_message("Failed to create/open V2Root registry key", __FILE__, __LINE__, GetLastError(), NULL);
            return;
        }
    }
    if (RegSetValueExA(hKey, "V2RayPID", 0, REG_DWORD, (const BYTE*)&pid, sizeof(pid)) != ERROR_SUCCESS) {
        log_message("Failed to save PID to registry", __FILE__, __LINE__, GetLastError(), NULL);
    }
    RegCloseKey(hKey);
}

/*
 * Loads the V2Ray process ID from the Windows registry under HKEY_CURRENT_USER\Software\V2Root.
 *
 * Returns:
 *   PID_TYPE: The stored PID, or 0 if not found or on error.
 *
 * Errors:
 *   Logs errors if registry read fails.
 */
__declspec(dllexport) PID_TYPE load_pid_from_registry() {
    HKEY hKey;
    PID_TYPE pid = 0;
    DWORD size = sizeof(pid);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\V2Root", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "V2RayPID", NULL, NULL, (LPBYTE)&pid, &size) != ERROR_SUCCESS) {
            log_message("Failed to load PID from registry", __FILE__, __LINE__, GetLastError(), NULL);
        }
        RegCloseKey(hKey);
    }
    return pid;
}

/*
 * Removes the V2Ray process ID from the Windows registry under HKEY_CURRENT_USER\Software\V2Root.
 *
 * Errors:
 *   None (silently fails if the key/value does not exist).
 */
__declspec(dllexport) void remove_pid_from_registry() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\V2Root", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, "V2RayPID");
        RegCloseKey(hKey);
    }
}

/*
 * Resets the system network proxy settings on Windows.
 *
 * Stops any running V2Ray process and clears proxy settings in the registry and WinHTTP.
 *
 * Returns:
 *   int: 0 on success.
 *
 * Errors:
 *   Logs errors if process termination or registry operations fail.
 */
__declspec(dllexport) int win_reset_network_proxy() {
    PID_TYPE pid_from_registry = load_pid_from_registry();
    if (pid_from_registry != 0) {
        if (win_stop_v2ray_process(pid_from_registry) != 0) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Failed to stop V2Ray process (PID: %lu) during proxy reset", (unsigned long)pid_from_registry);
            log_message(err_msg, __FILE__, __LINE__, GetLastError(), NULL);
        }
    }

    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_WRITE, &hKey);
    if (result == ERROR_SUCCESS) {
        DWORD disable = 0;
        RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&disable, sizeof(DWORD));
        RegDeleteValueA(hKey, "ProxyServer");
        RegDeleteValueA(hKey, "ProxyOverride");
        RegDeleteValueA(hKey, "AutoConfigURL");
        RegCloseKey(hKey);
    }

    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\V2Root");

    HINTERNET hSession = WinHttpOpen(L"V2Root Proxy Reset", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        WINHTTP_PROXY_INFO proxyInfo;
        proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        proxyInfo.lpszProxy = NULL;
        proxyInfo.lpszProxyBypass = NULL;
        WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo));
        WinHttpCloseHandle(hSession);
    }

    log_message("Network proxy reset executed", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Enables system proxy settings on Windows for HTTP and SOCKS protocols.
 *
 * Configures registry and WinHTTP for the specified ports.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port.
 *   socks_port (int): SOCKS proxy port.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if registry operations fail.
 */
__declspec(dllexport) int win_enable_system_proxy(int http_port, int socks_port) {
    HKEY hKey;
    char port_info[256];
    snprintf(port_info, sizeof(port_info), "Enabling proxy with HTTP Port: %d, SOCKS Port: %d", http_port, socks_port);
    log_message(port_info, __FILE__, __LINE__, 0, NULL);

    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        DWORD enable = 1;
        char proxy_str[64];
        snprintf(proxy_str, sizeof(proxy_str), "http=127.0.0.1:%d;socks=127.0.0.1:%d", http_port, socks_port);
        RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&enable, sizeof(DWORD));
        RegSetValueExA(hKey, "ProxyServer", 0, REG_SZ, (const BYTE*)proxy_str, strlen(proxy_str) + 1);
        RegCloseKey(hKey);

        HINTERNET hSession = WinHttpOpen(L"V2Root Proxy Enable", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (hSession) {
            WINHTTP_PROXY_INFO proxyInfo;
            proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            wchar_t proxy_wstr[64];
            swprintf(proxy_wstr, 64, L"http=127.0.0.1:%d;socks=127.0.0.1:%d", http_port, socks_port);
            proxyInfo.lpszProxy = proxy_wstr;
            proxyInfo.lpszProxyBypass = NULL;
            WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo));
            WinHttpCloseHandle(hSession);
        }

        char extra_info[256];
        snprintf(extra_info, sizeof(extra_info), "HTTP Port: %d, SOCKS Port: %d", http_port, socks_port);
        log_message("System proxy enabled", __FILE__, __LINE__, 0, extra_info);
        return 0;
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "RegOpenKeyExA failed with error: %ld", GetLastError());
        log_message(error_msg, __FILE__, __LINE__, 0, NULL);
        return -1;
    }
}

/*
 * Disables system proxy settings on Windows.
 *
 * Clears proxy settings from registry and WinHTTP.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if registry operations fail.
 */
__declspec(dllexport) int win_disable_system_proxy() {
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_WRITE, &hKey);
    if (result == ERROR_SUCCESS) {
        DWORD disable = 0;
        RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&disable, sizeof(DWORD));
        RegDeleteValueA(hKey, "ProxyServer");
        RegDeleteValueA(hKey, "ProxyOverride");
        RegDeleteValueA(hKey, "AutoConfigURL");
        log_message("System proxy disabled", __FILE__, __LINE__, 0, NULL);
        RegCloseKey(hKey);
    } else {
        log_message("Failed to open registry for disabling proxy", __FILE__, __LINE__, GetLastError(), NULL);
        return -1;
    }

    HINTERNET hSession = WinHttpOpen(L"V2Root Proxy Disable", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        WINHTTP_PROXY_INFO proxyInfo;
        proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        proxyInfo.lpszProxy = NULL;
        proxyInfo.lpszProxyBypass = NULL;
        WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo));
        WinHttpCloseHandle(hSession);
    }

    return 0;
}

/*
 * Starts a V2Ray process on Windows using the specified configuration file and executable path.
 *
 * Launches v2ray.exe and saves the process ID to the registry.
 *
 * Parameters:
 *   config_file (const char*): Path to the V2Ray configuration file.
 *   v2ray_path (const char*): Path to the V2Ray executable.
 *   pid (DWORD*): Pointer to store the process ID.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors if process creation fails.
 */
__declspec(dllexport) int win_start_v2ray_process(const char* config_file, const char* v2ray_path, DWORD* pid) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    char cmdline[MAX_PATH + 30];

    snprintf(cmdline, sizeof(cmdline), "\"%s\" run -config \"%s\"", v2ray_path, config_file);

    si.dwFlags = STARTF_USESTDHANDLES;
    HANDLE nul_handle = CreateFileA("nul", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    si.hStdInput = si.hStdOutput = si.hStdError = nul_handle;

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
        DWORD error = GetLastError();
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to start v2ray process (Code: %lu)", error);
        log_message(err_msg, __FILE__, __LINE__, error, cmdline);
        CloseHandle(nul_handle);
        return -1;
    }

    *pid = pi.dwProcessId;
    CloseHandle(nul_handle);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    save_pid_to_registry(*pid);

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Started v2ray with PID: %lu, saved to registry", *pid);
    log_message("Starting v2ray process", __FILE__, __LINE__, 0, extra_info);
    return 0;
}

/*
 * Stops a V2Ray process on Windows using the specified process ID.
 *
 * Terminates the process and removes its PID from the registry.
 *
 * Parameters:
 *   pid (DWORD): Process ID to stop.
 *
 * Returns:
 *   int: 0 on success or if process does not exist, -1 on failure.
 *
 * Errors:
 *   Logs errors if process termination fails.
 */
__declspec(dllexport) int win_stop_v2ray_process(DWORD pid) {
    if (pid == 0) {
        log_message("Invalid PID (0) provided", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER) {
            log_message("Process does not exist or invalid PID", __FILE__, __LINE__, error, NULL);
            remove_pid_from_registry();
            return 0;
        }
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to open process (PID: %lu)", (unsigned long)pid);
        log_message(err_msg, __FILE__, __LINE__, error, NULL);
        return -1;
    }

    if (!TerminateProcess(hProcess, 0)) {
        log_message("Failed to terminate V2Ray process", __FILE__, __LINE__, GetLastError(), NULL);
        CloseHandle(hProcess);
        return -1;
    }

    WaitForSingleObject(hProcess, 10000);
    CloseHandle(hProcess);
    remove_pid_from_registry();
    log_message("V2Ray process stopped successfully", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Tests the connectivity and latency of a V2Ray configuration on Windows.
 *
 * Sends an HTTP GET request to api.myip.com through the specified port.
 *
 * Parameters:
 *   http_port (int): HTTP proxy port.
 *   latency (int*): Pointer to store the latency in milliseconds.
 *   hProcess (HANDLE): Handle to the V2Ray process.
 *
 * Returns:
 *   int: 0 on success, -3 for connection errors, -4 for timeout.
 *
 * Errors:
 *   Logs errors if WinHTTP operations fail.
 */
__declspec(dllexport) int win_test_connection(int http_port, int* latency, HANDLE hProcess) {
    HINTERNET hSession = WinHttpOpen(L"V2Ray Test", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        DWORD error = GetLastError();
        log_message("WinHttpOpen failed", __FILE__, __LINE__, error, NULL);
        TerminateProcess(hProcess, 0);
        return -3;
    }

    WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.myip.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        DWORD error = GetLastError();
        log_message("WinHttpConnect failed", __FILE__, __LINE__, error, NULL);
        WinHttpCloseHandle(hSession);
        return -3;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        DWORD error = GetLastError();
        log_message("WinHttpOpenRequest failed", __FILE__, __LINE__, error, NULL);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return -3;
    }

    LARGE_INTEGER start, end, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    BOOL bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (bResult) {
        bResult = WinHttpReceiveResponse(hRequest, NULL);
    }
    QueryPerformanceCounter(&end);

    if (bResult) {
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &dwStatusCode, &dwSize, NULL);
        if (dwStatusCode == 200) {
            *latency = (int)((end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart);
            char extra_info[256];
            snprintf(extra_info, sizeof(extra_info), "Latency: %dms, Status: %lu", *latency, dwStatusCode);
            log_message("Connection test successful", __FILE__, __LINE__, 0, extra_info);
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 0;
        }
    }

    DWORD dwError = GetLastError();
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "bResult: %d, WinHTTP Error: %lu", bResult, dwError);
    log_message("Connection test failed", __FILE__, __LINE__, dwError, extra_info);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (dwError == ERROR_WINHTTP_TIMEOUT) return -4;
    return -3;
}