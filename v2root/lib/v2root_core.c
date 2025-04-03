/*
 * V2ROOT Core Library
 * Version: 1.0
 * Created: April 2025
 * Author: Sepehr0Day
 * Description: A lightweight C library designed to interface with the v2ray proxy software. This library provides
 *              functionality to load v2ray configurations, start and stop the v2ray process, test server connections,
 *              parse VLESS strings into v2ray config files, and check server status. It is intended for use in
 *              Windows environments and integrates with the system registry and proxy settings.
 * Dependencies: 
 *   - Windows API (Winsock2 for networking, Registry for PID management)
 *   - v2ray executable (v2ray.exe, expected at ".\\lib\\v2ray.exe" relative to the DLL's directory)
 * Target Platform: Windows (tested on Windows 10 and above)
 * Compilation: Use a Windows-compatible C compiler (e.g., MinGW-w64 or MSVC) with command:
 *              `x86_64-w64-mingw32-gcc -shared -o libv2root.dll v2root_core.c -lws2_32 -lwininet`
 *              or `cl /LD v2root_core.c ws2_32.lib wininet.lib` (MSVC)
 * License: MIT
 * 
 * MIT License:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions 
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

 #include <winsock2.h>
 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <wininet.h>
 #include "v2root_core.h"
 
 #define V2RAY_BINARY ".\\lib\\v2ray.exe"
 /*
  * Path to the v2ray executable. This macro specifies the default location where the library expects to find v2ray.exe,
  * relative to the current working directory when the DLL is executed.
  */
 
 #define REG_KEY "Software\\V2Root"
 /*
  * Registry key path under HKEY_CURRENT_USER where v2ray process information (PID) is stored.
  */
 
 #define REG_VALUE_PID "V2Root"
 /*
  * Registry value name that stores the process ID (PID) of the running v2ray instance as a DWORD.
  */
 
 #define DEFAULT_HTTP_PORT 2300
 /*
  * Default local HTTP proxy port used when enabling the system proxy. Fixed at 2300 and cannot be changed by the user.
  */
 
 #define DEFAULT_SOCKS_PORT 2301
 /*
  * Default local SOCKS proxy port used when enabling the system proxy. Fixed at 2301 and cannot be changed by the user.
  */
 
 static int GetPIDFromRegistry() {
     HKEY hKey;
     DWORD pid = 0;
     DWORD size = sizeof(DWORD);
 
     if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
         RegQueryValueExA(hKey, REG_VALUE_PID, NULL, NULL, (LPBYTE)&pid, &size);
         RegCloseKey(hKey);
     }
     return (int)pid;
 }
 /*
  * Retrieves the process ID (PID) of the currently running v2ray instance from the Windows registry.
  *
  * Behavior:
  * - Opens the registry key specified by REG_KEY with read access.
  * - Queries the REG_VALUE_PID value, which stores the PID as a DWORD (32-bit unsigned integer).
  * - Closes the registry key after retrieval to free resources.
  *
  * Returns:
  * - int: The PID stored in the registry. Returns 0 if no PID is found, or if the key/value doesn’t exist or is inaccessible.
  */
 
 static void SavePIDToRegistry(int pid) {
     HKEY hKey;
     if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
         RegSetValueExA(hKey, REG_VALUE_PID, 0, REG_DWORD, (const BYTE*)&pid, sizeof(DWORD));
         RegCloseKey(hKey);
     }
 }
 /*
  * Saves the process ID (PID) of a v2ray instance to the Windows registry.
  *
  * Parameters:
  * - pid (int): The process ID to save. Can be 0 to clear the entry.
  *
  * Behavior:
  * - Creates or opens the registry key specified by REG_KEY with write access. If the key doesn’t exist, it’s created.
  * - Sets the REG_VALUE_PID value as a DWORD with the provided PID.
  * - Closes the registry key after writing to free resources.
  *
  * Notes:
  * - Uses REG_OPTION_NON_VOLATILE to ensure the key persists across reboots.
  */
 
 static void EnableSystemProxy() {
     HKEY hKey;
     if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
         DWORD enable = 1;
         char proxy_str[64];
         snprintf(proxy_str, sizeof(proxy_str), "http=127.0.0.1:%d;socks=127.0.0.1:%d", DEFAULT_HTTP_PORT, DEFAULT_SOCKS_PORT);
         RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&enable, sizeof(DWORD));
         RegSetValueExA(hKey, "ProxyServer", 0, REG_SZ, (const BYTE*)proxy_str, strlen(proxy_str) + 1);
         RegCloseKey(hKey);
     }
 }
 /*
  * Enables the system-wide proxy settings in Windows using fixed HTTP and SOCKS ports.
  *
  * Behavior:
  * - Opens the Internet Settings registry key with write access.
  * - Sets ProxyEnable to 1 to enable the proxy.
  * - Constructs a proxy string in the format "httpлады127.0.0.1:2300;socks=127.0.0.1:2301" using DEFAULT_HTTP_PORT and DEFAULT_SOCKS_PORT.
  * - Sets the ProxyServer value with the constructed string.
  * - Closes the registry key after writing.
  *
  * Notes:
  * - Proxy ports are hardcoded and cannot be customized by the user.
  */
 
 __declspec(dllexport) void DisableSystemProxy() {
     HKEY hKey;
     if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
         DWORD disable = 0;
         RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&disable, sizeof(DWORD));
         RegCloseKey(hKey);
     }
 }
 /*
  * Disables the system-wide proxy settings in Windows.
  *
  * Behavior:
  * - Opens the Internet Settings registry key with write access.
  * - Sets ProxyEnable to 0 to disable the proxy.
  * - Closes the registry key after writing.
  *
  * Notes:
  * - Does not modify the ProxyServer value, as it’s ignored when ProxyEnable is 0.
  */
 
 __declspec(dllexport) void ResetNetworkProxy() {
     stop_v2ray();
 
     HKEY hKey;
     LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_WRITE, &hKey);
     if (result == ERROR_SUCCESS) {
         DWORD disable = 0;
         result = RegSetValueExA(hKey, "ProxyEnable", 0, REG_DWORD, (const BYTE*)&disable, sizeof(DWORD));
         if (result == ERROR_SUCCESS) {
             RegDeleteValueA(hKey, "ProxyServer");
             RegDeleteValueA(hKey, "ProxyOverride");
             RegDeleteValueA(hKey, "AutoConfigURL");
         }
         RegCloseKey(hKey);
     }
 
     RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\V2Root");
 
     InternetSetOptionA(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
     InternetSetOptionA(NULL, INTERNET_OPTION_REFRESH, NULL, 0);
 }
 /*
  * Fully resets all changes made by V2Root on the system, including proxy settings and registry entries.
  *
  * Behavior:
  * - Stops the v2ray process if it’s running (via stop_v2ray).
  * - Opens the Internet Settings registry key with write access.
  * - Sets ProxyEnable to 0 to disable the proxy.
  * - Deletes ProxyServer, ProxyOverride, and AutoConfigURL to clear all proxy configurations.
  * - Deletes the V2Root registry key to remove any stored PID.
  * - Notifies Windows to refresh proxy settings using InternetSetOptionA.
  *
  * Notes:
  * - Ensures all modifications made by V2Root (proxy settings, registry entries, running process) are reverted.
  * - Ignores errors from RegDeleteValueA and RegDeleteKeyA if the values/keys don’t exist.
  */
 
 __declspec(dllexport) int load_v2ray_config(void) {
     if (GetFileAttributesA("config.json") == INVALID_FILE_ATTRIBUTES) {
         return -1;
     }
     return 0;
 }
 /*
  * Loads and validates the fixed v2ray configuration file "config.json".
  *
  * Parameters:
  * - None
  *
  * Behavior:
  * - Checks if the file "config.json" exists and is accessible in the current working directory using GetFileAttributesA.
  * - Returns immediately if the file is valid; does not parse or validate the content beyond existence.
  *
  * Returns:
  * - 0: Success (file "config.json" exists and is accessible).
  * - -1: Failure (file doesn’t exist or is inaccessible).
  *
  * Notes:
  * - The configuration file is hardcoded to "config.json" and cannot be changed by the user.
  */
 
 __declspec(dllexport) int start_v2ray() {
     int current_pid = GetPIDFromRegistry();
     if (current_pid > 0) {
         HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, current_pid);
         if (hProcess) {
             DWORD exitCode;
             if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
                 CloseHandle(hProcess);
                 return -2;
             }
             CloseHandle(hProcess);
         }
         SavePIDToRegistry(0);
     }
 
     char dll_path[MAX_PATH];
     GetModuleFileNameA(GetModuleHandleA("libv2root.dll"), dll_path, MAX_PATH);
     char* last_slash = strrchr(dll_path, '\\');
     if (last_slash) *last_slash = '\0';
     strcat(dll_path, "\\v2ray.exe");
  
     char cmdline[MAX_PATH + 20];
     snprintf(cmdline, sizeof(cmdline), "\"%s\" run -config config.json", dll_path);
 
     STARTUPINFOA si = { sizeof(si) };
     PROCESS_INFORMATION pi;
 
     si.dwFlags = STARTF_USESTDHANDLES;
     HANDLE nul_handle = CreateFileA("nul", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
     si.hStdInput = nul_handle;
     si.hStdOutput = nul_handle;
     si.hStdError = nul_handle;
 
     if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
         CloseHandle(nul_handle);
         return -1;
     }
 
     CloseHandle(nul_handle);
     CloseHandle(pi.hThread);
     SavePIDToRegistry(pi.dwProcessId);
     EnableSystemProxy();
     return pi.dwProcessId;
 }
 /*
  * Starts the v2ray process using the loaded configuration.
  *
  * Behavior:
  * - Checks if a v2ray process is already running by retrieving its PID from the registry using GetPIDFromRegistry.
  * - If a process is active (PID > 0 and still running), returns -2.
  * - If no process is running or it’s terminated, clears the PID in the registry.
  * - Constructs the path to v2ray.exe based on the DLL’s location and launches it with the command line "<path>\v2ray.exe run -config config.json".
  * - Runs the process detached with no window and redirects standard I/O to "nul" to suppress output.
  * - Saves the new PID to the registry and enables the system proxy with fixed ports.
  *
  * Returns:
  * - Positive integer: The process ID (PID) of the newly started v2ray instance on success.
  * - -1: Failure (e.g., v2ray.exe not found or process creation failed).
  * - -2: v2ray is already running.
  */
 
 __declspec(dllexport) int stop_v2ray() {
     int pid = GetPIDFromRegistry();
     if (pid <= 0) {
         return -1;
     }
 
     HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, pid);
     if (hProcess == NULL) {
         SavePIDToRegistry(0);
         DisableSystemProxy();
         return -1;
     }
 
     DWORD exitCode;
     if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
         TerminateProcess(hProcess, 0);
         SavePIDToRegistry(0);
         DisableSystemProxy();
         CloseHandle(hProcess);
         return 0;
     } else {
         SavePIDToRegistry(0);
         DisableSystemProxy();
         CloseHandle(hProcess);
         return -1;
     }
 }
 /*
  * Stops the currently running v2ray process.
  *
  * Behavior:
  * - Retrieves the PID from the registry using GetPIDFromRegistry.
  * - If no valid PID exists (PID <= 0), returns -1.
  * - Opens the process with termination and query rights.
  * - If the process is still active, terminates it, clears the PID from the registry, and disables the system proxy.
  * - If the process is already terminated or inaccessible, clears the registry and proxy settings anyway.
  *
  * Returns:
  * - 0: Success (process terminated or no active process found).
  * - -1: Failure (no valid PID in registry or process access/termination failed).
  */
 
 __declspec(dllexport) int ping_server(const char* address, int port) {
     WSADATA wsaData;
     SOCKET sock;
     struct sockaddr_in server;
     LARGE_INTEGER start, end, frequency;
     double elapsed_ms;
 
     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return -1;
     sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (sock == INVALID_SOCKET) {
         WSACleanup();
         return -1;
     }
 
     server.sin_family = AF_INET;
     server.sin_addr.s_addr = inet_addr(address);
     server.sin_port = htons(port);
 
     QueryPerformanceFrequency(&frequency);
     QueryPerformanceCounter(&start);
 
     if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
         closesocket(sock);
         WSACleanup();
         return -1;
     }
 
     QueryPerformanceCounter(&end);
     elapsed_ms = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
 
     closesocket(sock);
     WSACleanup();
     return (int)elapsed_ms;
 }
 /*
  * Measures latency to a server by establishing a TCP connection.
  *
  * Parameters:
  * - address (const char*): A null-terminated string with the server IP address (e.g., "1.1.1.1").
  * - port (int): The port to connect to (e.g., 443).
  *
  * Behavior:
  * - Initializes Winsock with version 2.2.
  * - Creates a TCP socket.
  * - Sets up the server address structure with the provided IP and port.
  * - Measures the time taken to establish a connection using high-resolution timers.
  * - Closes the socket and cleans up Winsock resources.
  *
  * Returns:
  * - Positive integer: Latency in milliseconds if successful.
  * - -1: Failure (Winsock init failed, socket creation failed, or connection failed).
  *
  * Notes:
  * - Only supports IP addresses, not hostnames (no DNS resolution).
  */
 
 __declspec(dllexport) int parse_vless_string(const char* vless_str) {
     if (vless_str == NULL) return -1;
 
     char uuid[128] = "";
     char address[2048] = "";
     char port_str[16] = "";
     char params[4096] = "";
 
     if (sscanf(vless_str, "vless://%127[^@]@%2047[^:]:%15[^?]?%4095s", uuid, address, port_str, params) != 4) {
         return -1;
     }
 
     int server_port = atoi(port_str);
     if (server_port <= 0 || server_port > 65535) return -1;
 
     char encryption[128] = "none";
     char flow[128] = "";
     char network[128] = "tcp";
     char security[128] = "none";
     char header_type[128] = "none";
     char path[2048] = "";
     char host[2048] = "";
     char service_name[2048] = "";
     char grpc_mode[128] = "gun";
     char quic_security[128] = "none";
     char quic_key[2048] = "";
     char server_name[2048] = "";
     char fingerprint[128] = "";
     char public_key[2048] = "";
     char short_id[128] = "";
     char spider_x[2048] = "";
 
     char params_copy[4096];
     if (strlen(params) >= sizeof(params_copy)) return -1;
     strncpy(params_copy, params, sizeof(params_copy) - 1);
     params_copy[sizeof(params_copy) - 1] = '\0';
 
     char* param = strtok(params_copy, "&");
     while (param) {
         if (strncmp(param, "encryption=", 11) == 0) strncpy(encryption, param + 11, sizeof(encryption) - 1);
         else if (strncmp(param, "flow=", 5) == 0) strncpy(flow, param + 5, sizeof(flow) - 1);
         else if (strncmp(param, "type=", 5) == 0) strncpy(network, param + 5, sizeof(network) - 1);
         else if (strncmp(param, "security=", 9) == 0) strncpy(security, param + 9, sizeof(security) - 1);
         else if (strncmp(param, "headerType=", 11) == 0) strncpy(header_type, param + 11, sizeof(header_type) - 1);
         else if (strncmp(param, "path=", 5) == 0) strncpy(path, param + 5, sizeof(path) - 1);
         else if (strncmp(param, "host=", 5) == 0) strncpy(host, param + 5, sizeof(host) - 1);
         else if (strncmp(param, "serviceName=", 12) == 0) strncpy(service_name, param + 12, sizeof(service_name) - 1);
         else if (strncmp(param, "mode=", 5) == 0) strncpy(grpc_mode, param + 5, sizeof(grpc_mode) - 1);
         else if (strncmp(param, "quicSecurity=", 13) == 0) strncpy(quic_security, param + 13, sizeof(quic_security) - 1);
         else if (strncmp(param, "key=", 4) == 0) strncpy(quic_key, param + 4, sizeof(quic_key) - 1);
         else if (strncmp(param, "serverName=", 11) == 0) strncpy(server_name, param + 11, sizeof(server_name) - 1);
         else if (strncmp(param, "fp=", 3) == 0) strncpy(fingerprint, param + 3, sizeof(fingerprint) - 1);
         else if (strncmp(param, "pbk=", 4) == 0) strncpy(public_key, param + 4, sizeof(public_key) - 1);
         else if (strncmp(param, "sid=", 4) == 0) strncpy(short_id, param + 4, sizeof(short_id) - 1);
         else if (strncmp(param, "spx=", 4) == 0) strncpy(spider_x, param + 4, sizeof(spider_x) - 1);
         param = strtok(NULL, "&");
     }
 
     FILE* fp = fopen("config.json", "w");
     if (!fp) return -1;
 
     fprintf(fp, "{\n");
     fprintf(fp, "  \"inbounds\": [\n");
     fprintf(fp, "    {\"port\": %d, \"protocol\": \"http\", \"settings\": {}},\n", DEFAULT_HTTP_PORT);
     fprintf(fp, "    {\"port\": %d, \"protocol\": \"socks\", \"settings\": {\"udp\": true}}\n", DEFAULT_SOCKS_PORT);
     fprintf(fp, "  ],\n");
     fprintf(fp, "  \"outbounds\": [{\n");
     fprintf(fp, "    \"protocol\": \"vless\",\n");
     fprintf(fp, "    \"settings\": {\"vnext\": [{\"address\": \"%s\", \"port\": %d, \"users\": [{\"id\": \"%s\", \"encryption\": \"%s\"", address, server_port, uuid, encryption);
     if (flow[0]) fprintf(fp, ", \"flow\": \"%s\"", flow);
     fprintf(fp, "}]}]},\n");
 
     fprintf(fp, "    \"streamSettings\": {\n");
     fprintf(fp, "      \"network\": \"%s\",\n", network);
     fprintf(fp, "      \"security\": \"%s\",\n", security);
 
     if (strcmp(network, "tcp") == 0) {
         fprintf(fp, "      \"tcpSettings\": {\"header\": {\"type\": \"%s\"}}\n", header_type);
     } else if (strcmp(network, "http") == 0) {
         fprintf(fp, "      \"httpSettings\": {");
         if (path[0]) fprintf(fp, "\"path\": \"%s\"", path);
         if (path[0] && host[0]) fprintf(fp, ", ");
         if (host[0]) {
             fprintf(fp, "\"host\": [");
             char host_copy[2048];
             strncpy(host_copy, host, sizeof(host_copy) - 1);
             host_copy[sizeof(host_copy) - 1] = '\0';
             char* h = strtok(host_copy, ",");
             int first = 1;
             while (h) {
                 if (!first) fprintf(fp, ", ");
                 fprintf(fp, "\"%s\"", h);
                 first = 0;
                 h = strtok(NULL, ",");
             }
             fprintf(fp, "]");
         }
         fprintf(fp, "}\n");
     } else if (strcmp(network, "ws") == 0) {
         fprintf(fp, "      \"wsSettings\": {");
         if (path[0]) fprintf(fp, "\"path\": \"%s\"", path);
         if (path[0] && host[0]) fprintf(fp, ", ");
         if (host[0]) fprintf(fp, "\"headers\": {\"Host\": \"%s\"}", host);
         fprintf(fp, "}\n");
     } else if (strcmp(network, "grpc") == 0) {
         fprintf(fp, "      \"grpcSettings\": {");
         if (service_name[0]) fprintf(fp, "\"serviceName\": \"%s\"", service_name);
         if (service_name[0] && grpc_mode[0]) fprintf(fp, ", ");
         if (grpc_mode[0]) fprintf(fp, "\"mode\": \"%s\"", grpc_mode);
         fprintf(fp, "}\n");
     } else if (strcmp(network, "quic") == 0) {
         fprintf(fp, "      \"quicSettings\": {");
         fprintf(fp, "\"security\": \"%s\"", quic_security);
         if (quic_key[0]) fprintf(fp, ", \"key\": \"%s\"", quic_key);
         fprintf(fp, "}\n");
     }
 
     if (strcmp(security, "tls") == 0 || strcmp(security, "xtls") == 0 || strcmp(security, "reality") == 0) {
         fprintf(fp, "      ,\"%sSettings\": {", security);
         if (server_name[0]) fprintf(fp, "\"serverName\": \"%s\"", server_name);
         if (server_name[0] && fingerprint[0]) fprintf(fp, ", ");
         if (fingerprint[0]) fprintf(fp, "\"fingerprint\": \"%s\"", fingerprint);
         if (strcmp(security, "reality") == 0) {
             if ((server_name[0] || fingerprint[0]) && public_key[0]) fprintf(fp, ", ");
             if (public_key[0]) fprintf(fp, "\"publicKey\": \"%s\"", public_key);
             if (public_key[0] && short_id[0]) fprintf(fp, ", ");
             if (short_id[0]) fprintf(fp, "\"shortId\": \"%s\"", short_id);
             if (short_id[0] && spider_x[0]) fprintf(fp, ", ");
             if (spider_x[0]) fprintf(fp, "\"spiderX\": \"%s\"", spider_x);
         }
         fprintf(fp, "}\n");
     }
 
     fprintf(fp, "    }\n");
     fprintf(fp, "  }]\n");
     fprintf(fp, "}\n");
 
     fclose(fp);
     EnableSystemProxy();
     return 0;
 }
 /*
  * Parses a VLESS string and generates a v2ray configuration file named "config.json".
  *
  * Parameters:
  * - vless_str (const char*): A null-terminated VLESS string (e.g., "vless://uuid@address:port?params").
  *
  * Behavior:
  * - Validates input parameter for null value.
  * - Parses the VLESS string into mandatory components (uuid, address, port) and optional parameters.
  * - Supports all VLESS settings: encryption, flow, network type (tcp, http, ws, grpc, quic), security (none, tls, xtls, reality),
  *   and related options like headerType, path, host, serviceName, mode, quicSecurity, key, serverName, fingerprint, publicKey,
  *   shortId, and spiderX.
  * - Writes a JSON config file named "config.json" with fixed inbound ports (HTTP 2300, SOCKS 2301) and outbound VLESS settings.
  * - Enables the system proxy with fixed ports after successful file creation.
  *
  * Returns:
  * - 0: Success (config file written and proxy enabled).
  * - -1: Failure (invalid VLESS string, null parameter, port out of range, or file write error).
  */
 
 __declspec(dllexport) int check_server_status(const char* vless_str) {
     if (vless_str == NULL) return -1;
 
     char address[2048] = "";
     char port_str[16] = "";
     if (sscanf(vless_str, "vless://%*[^@]@%2047[^:]:%15[^?]", address, port_str) != 2) return -1;
 
     int port = atoi(port_str);
     if (port <= 0 || port > 65535) return -1;
 
     WSADATA wsaData;
     SOCKET sock;
     struct sockaddr_in server;
     LARGE_INTEGER start, end, frequency;
     double elapsed_ms;
 
     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
         return -2;
     }
 
     sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (sock == INVALID_SOCKET) {
         WSACleanup();
         return -3;
     }
 
     struct hostent* host = gethostbyname(address);
     if (host == NULL) {
         closesocket(sock);
         WSACleanup();
         return -4;
     }
 
     server.sin_family = AF_INET;
     server.sin_addr.s_addr = *(unsigned long*)host->h_addr;
     server.sin_port = htons(port);
 
     QueryPerformanceFrequency(&frequency);
     QueryPerformanceCounter(&start);
 
     if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
         closesocket(sock);
         WSACleanup();
         return -5;
     }
 
     QueryPerformanceCounter(&end);
     elapsed_ms = (double)(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
 
     closesocket(sock);
     WSACleanup();
     return (int)elapsed_ms;
 }
 /*
  * Checks the status of a VLESS server and returns the connection latency in milliseconds.
  *
  * Parameters:
  * - vless_str (const char*): A null-terminated VLESS string (e.g., "vless://uuid@address:port?params").
  *
  * Behavior:
  * - Validates input parameter for null value.
  * - Extracts address and port from the VLESS string.
  * - Initializes Winsock and creates a TCP socket.
  * - Resolves the hostname to an IP address using DNS.
  * - Measures the time to establish a TCP connection using high-resolution timers.
  * - Cleans up resources (socket and Winsock).
  *
  * Returns:
  * - Positive integer: Latency in milliseconds if the connection is successful.
  * - -1: Invalid input (null parameter, invalid VLESS format, or port out of range).
  * - -2: Winsock initialization failed.
  * - -3: Socket creation failed.
  * - -4: DNS resolution failed.
  * - -5: Connection timeout.
  */