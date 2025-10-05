#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>

/* Include winhttp.h first - needed for TTFB measurement */
#include <winhttp.h>

/* We need InternetSetOption functions but there are type conflicts between wininet.h and winhttp.h
   So we'll manually declare just what we need from wininet.h */
#define INTERNET_OPTION_SETTINGS_CHANGED 39
#define INTERNET_OPTION_REFRESH 37

/* Declare the InternetSetOptionA function */
BOOL WINAPI InternetSetOptionA(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength);

#include <stdio.h>
#include <string.h>
#include "libv2root_win.h"
#include "libv2root_utils.h"

#define REGISTRY_KEY "Software\\V2ROOT"
#define REGISTRY_PID_VALUE "V2RayPID"

/* Function pointer type for InternetSetOptionA */
typedef BOOL (WINAPI *InternetSetOptionA_t)(HINTERNET, DWORD, LPVOID, DWORD);

/* Helper function to notify the system about proxy changes */
static void notify_proxy_change(void) {
    HMODULE wininet = LoadLibraryA("wininet.dll");
    if (!wininet) {
        log_message("Failed to load wininet.dll", __FILE__, __LINE__, GetLastError(), NULL);
        return;
    }
    
    InternetSetOptionA_t pInternetSetOptionA = 
        (InternetSetOptionA_t)GetProcAddress(wininet, "InternetSetOptionA");
        
    if (pInternetSetOptionA) {
        pInternetSetOptionA(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
        pInternetSetOptionA(NULL, INTERNET_OPTION_REFRESH, NULL, 0);
    } else {
        log_message("Failed to get InternetSetOptionA function", __FILE__, __LINE__, GetLastError(), NULL);
    }
    
    FreeLibrary(wininet);
}

/*
 * Starts V2Ray process using CreateProcess.
 */
int win_start_v2ray_process(const char* config_file, const char* v2ray_path, DWORD* pid) {
    if (!config_file || !v2ray_path || !pid) {
        log_message("Invalid arguments to win_start_v2ray_process", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    ZeroMemory(&pi, sizeof(pi));
    
    char cmdLine[2048];
    snprintf(cmdLine, sizeof(cmdLine), "\"%s\" run -c \"%s\"", v2ray_path, config_file);
    
    if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        DWORD error = GetLastError();
        log_message("Failed to create V2Ray process", __FILE__, __LINE__, error, cmdLine);
        return -1;
    }
    
    *pid = pi.dwProcessId;
    
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    
    Sleep(500); /* Wait 500ms for process to start */
    
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "V2Ray started with PID: %lu", *pid);
    log_message("Windows V2Ray process started", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Stops V2Ray process by terminating it.
 */
int win_stop_v2ray_process(DWORD pid) {
    if (pid == 0) {
        /* Silently succeed - no output to terminal, just log */
        log_message("Invalid PID (zero) for stop - ignoring", __FILE__, __LINE__, 0, NULL);
        return 0;
    }
    
    /* First verify if the process exists and we can access it */
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER) {
            /* Process not found - silently succeed, only log to file */
            log_message("Process not found (Invalid PID) - already terminated", __FILE__, __LINE__, 0, NULL);
            return 0;
        } else if (error == ERROR_ACCESS_DENIED) {
            log_message("Access denied to process", __FILE__, __LINE__, error, NULL);
            return -1;
        } else {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Failed to open process for query: %lu", error);
            log_message(err_msg, __FILE__, __LINE__, error, NULL);
            return -1;
        }
    }
    
    /* Check if process is still running */
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(hProcess, &exitCode)) {
        DWORD error = GetLastError();
        log_message("Failed to get process exit code", __FILE__, __LINE__, error, NULL);
        CloseHandle(hProcess);
        return -1;
    }
    
    if (exitCode != STILL_ACTIVE) {
        /* Process already terminated - silently succeed, only log */
        log_message("Process already terminated - no action needed", __FILE__, __LINE__, 0, NULL);
        CloseHandle(hProcess);
        return 0;
    }
    
    /* Close the query handle and reopen with termination rights */
    CloseHandle(hProcess);
    
    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER) {
            /* Process disappeared between our checks - silently succeed */
            log_message("Process not found when reopening - already terminated", __FILE__, __LINE__, 0, NULL);
            return 0;
        }
        log_message("Failed to open process for termination", __FILE__, __LINE__, error, NULL);
        return -1;
    }
    
    if (!TerminateProcess(hProcess, 0)) {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            log_message("Access denied when terminating process", __FILE__, __LINE__, error, NULL);
        } else {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Failed to terminate process: %lu", error);
            log_message(err_msg, __FILE__, __LINE__, error, NULL);
        }
        CloseHandle(hProcess);
        return -1;
    }
    
    /* Wait for process to terminate with timeout */
    DWORD waitResult = WaitForSingleObject(hProcess, 5000);
    if (waitResult == WAIT_TIMEOUT) {
        log_message("Process termination timed out", __FILE__, __LINE__, 0, NULL);
    } else if (waitResult == WAIT_FAILED) {
        log_message("WaitForSingleObject failed", __FILE__, __LINE__, GetLastError(), NULL);
    }
    
    CloseHandle(hProcess);
    log_message("V2Ray process terminated successfully", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Saves PID to Windows Registry.
 */
void save_pid_to_registry(DWORD pid) {
    HKEY hKey;
    
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, REGISTRY_PID_VALUE, 0, REG_DWORD, (BYTE*)&pid, sizeof(DWORD));
        RegCloseKey(hKey);
        log_message("PID saved to registry", __FILE__, __LINE__, 0, NULL);
    } else {
        log_message("Failed to save PID to registry", __FILE__, __LINE__, GetLastError(), NULL);
    }
}

/*
 * Loads PID from Windows Registry.
 */
DWORD load_pid_from_registry(void) {
    HKEY hKey;
    DWORD pid = 0;
    DWORD dataSize = sizeof(DWORD);
    
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, REGISTRY_PID_VALUE, NULL, NULL, (BYTE*)&pid, &dataSize) != ERROR_SUCCESS) {
            pid = 0;
        }
        RegCloseKey(hKey);
    }
    
    return pid;
}

/*
 * Enables Windows system proxy via Internet Settings.
 */
int win_enable_system_proxy(int http_port, int socks_port) {
    HKEY hKey;
    
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                      0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        log_message("Failed to open Internet Settings registry key", __FILE__, __LINE__, GetLastError(), NULL);
        return -1;
    }
    
    DWORD enable = 1;
    RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (BYTE*)&enable, sizeof(DWORD));
    
    char proxy[128];
    snprintf(proxy, sizeof(proxy), "http=127.0.0.1:%d;https=127.0.0.1:%d;socks=127.0.0.1:%d",
             http_port, http_port, socks_port);
    
    RegSetValueExA(hKey, "ProxyServer", 0, REG_SZ, (BYTE*)proxy, (DWORD)strlen(proxy) + 1);
    RegCloseKey(hKey);
    
    /* Notify Internet Explorer of proxy change */
    notify_proxy_change();
    
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Proxy: %s", proxy);
    log_message("Windows system proxy enabled", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Disables Windows system proxy.
 */
int win_disable_system_proxy(void) {
    HKEY hKey;
    
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                      0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        log_message("Failed to open Internet Settings registry key", __FILE__, __LINE__, GetLastError(), NULL);
        return -1;
    }
    
    DWORD disable = 0;
    RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (BYTE*)&disable, sizeof(DWORD));
    RegCloseKey(hKey);
    
    /* Notify Internet Explorer of proxy change */
    notify_proxy_change();
    
    log_message("Windows system proxy disabled", __FILE__, __LINE__, 0, NULL);
    return 0;
}

/*
 * Tests connection latency through Windows proxy by making a real HTTP request.
 * This measures actual end-to-end latency through the V2Ray node.
 */
int win_test_connection(int http_port, int* latency, HANDLE hProcess) {
    if (!latency) {
        log_message("Null latency pointer", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    
    // Configure proxy settings for WinHTTP
    WINHTTP_PROXY_INFO proxyInfo;
    ZeroMemory(&proxyInfo, sizeof(proxyInfo));
    proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    
    char proxy_str[128];
    snprintf(proxy_str, sizeof(proxy_str), "http://127.0.0.1:%d", http_port);
    wchar_t proxy_wide[128];
    MultiByteToWideChar(CP_UTF8, 0, proxy_str, -1, proxy_wide, 128);
    proxyInfo.lpszProxy = proxy_wide;
    proxyInfo.lpszProxyBypass = L"<local>";
    
    // Open WinHTTP session
    HINTERNET hSession = WinHttpOpen(
        L"V2Root-Test/1.0",
        WINHTTP_ACCESS_TYPE_NAMED_PROXY,
        proxy_wide,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!hSession) {
        log_message("Failed to open WinHTTP session", __FILE__, __LINE__, GetLastError(), NULL);
        return -1;
    }
    
    // Set timeouts (10 seconds total)
    DWORD timeout = 10000;
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    // Connect to test endpoint (Google's 204 endpoint)
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"www.google.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );
    
    if (!hConnect) {
        log_message("Failed to connect via WinHTTP", __FILE__, __LINE__, GetLastError(), NULL);
        WinHttpCloseHandle(hSession);
        return -1;
    }
    
    // Start timing
    QueryPerformanceCounter(&start);
    
    // Open HTTP request
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        L"/generate_204",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    
    if (!hRequest) {
        log_message("Failed to create WinHTTP request", __FILE__, __LINE__, GetLastError(), NULL);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return -1;
    }
    
    // Send request
    BOOL bResult = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );
    
    if (!bResult) {
        log_message("Failed to send WinHTTP request", __FILE__, __LINE__, GetLastError(), NULL);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return -1;
    }
    
    // Receive response
    bResult = WinHttpReceiveResponse(hRequest, NULL);
    
    // Stop timing immediately after receiving response headers
    QueryPerformanceCounter(&end);
    
    if (!bResult) {
        log_message("Failed to receive WinHTTP response", __FILE__, __LINE__, GetLastError(), NULL);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return -1;
    }
    
    // Calculate latency
    double elapsed_ms = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / (double)freq.QuadPart;
    *latency = (int)(elapsed_ms + 0.5);
    
    // Ensure minimum 1ms
    if (*latency < 1) {
        *latency = 1;
    }
    
    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Real connection latency: %d ms (actual: %.2f ms)", *latency, elapsed_ms);
    log_message("Connection test successful via proxy", __FILE__, __LINE__, 0, extra_info);
    
    return 0;
}

/*
 * Performs a single HTTP request through the V2Ray proxy and measures TTFB.
 * Returns JSON with platform, success, ttfb_ms, http_status, and error_message.
 */
EXPORT char* win_measure_ttfb(int http_port) {
    static char result[1024];
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    
    // Configure proxy settings for WinHTTP
    char proxy_str[128];
    snprintf(proxy_str, sizeof(proxy_str), "http://127.0.0.1:%d", http_port);
    wchar_t proxy_wide[128];
    MultiByteToWideChar(CP_UTF8, 0, proxy_str, -1, proxy_wide, 128);
    
    // Open WinHTTP session
    HINTERNET hSession = WinHttpOpen(
        L"V2Root-TTFBTest/1.0",
        WINHTTP_ACCESS_TYPE_NAMED_PROXY,
        proxy_wide,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    
    if (!hSession) {
        snprintf(result, sizeof(result),
                 "{\"platform\": \"windows\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to open HTTP session: %lu\"}", 
                 GetLastError());
        return result;
    }
    
    // Set timeouts (10 seconds total)
    DWORD timeout = 10000;
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    
    // Connect to Google
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"www.google.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );
    
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        snprintf(result, sizeof(result),
                 "{\"platform\": \"windows\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to connect: %lu\"}", 
                 GetLastError());
        return result;
    }
    
    // Create HTTP request
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        L"/generate_204",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        snprintf(result, sizeof(result),
                 "{\"platform\": \"windows\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to create request: %lu\"}", 
                 GetLastError());
        return result;
    }
    
    // Start timing
    QueryPerformanceCounter(&start);
    
    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        snprintf(result, sizeof(result),
                 "{\"platform\": \"windows\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to send request: %lu\"}", 
                 GetLastError());
        return result;
    }
    
    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        snprintf(result, sizeof(result),
                 "{\"platform\": \"windows\", \"success\": false, \"ttfb_ms\": null, \"http_status\": null, \"error_message\": \"Failed to receive response: %lu\"}", 
                 GetLastError());
        return result;
    }
    
    // Stop timing at TTFB
    QueryPerformanceCounter(&end);
    double elapsed_ms = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / (double)freq.QuadPart;
    
    // Get HTTP status code
    DWORD status_code = 0;
    DWORD size = sizeof(DWORD);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status_code,
        &size,
        WINHTTP_NO_HEADER_INDEX
    );
    
    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    // Return formatted result
    snprintf(result, sizeof(result),
             "{\"platform\": \"windows\", \"success\": true, \"ttfb_ms\": %d, \"http_status\": %lu, \"error_message\": null}", 
             (int)(elapsed_ms + 0.5), status_code);
    
    return result;
}

#endif /* _WIN32 */