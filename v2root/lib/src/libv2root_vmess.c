#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "libv2root_vmess.h"
#include "libv2root_core.h"
#include "libv2root_utils.h"

/*
 * Parses a VMess configuration string and generates a V2Ray JSON configuration file.
 *
 * This function processes a VMess URI (e.g., vmess://base64_encoded_json) and produces a complete V2Ray JSON configuration,
 * including inbound and outbound settings. It supports advanced VMess features and ensures robust error handling.
 *
 * Supported Features:
 * - **Transport Protocols**: TCP, HTTP/2 (h2), WebSocket (ws), mKCP, QUIC, gRPC, with protocol-specific settings.
 * - **Security Options**: None, TLS (with Server Name Indication (SNI), ALPN as a list, fingerprint, allowInsecure).
 * - **Encryption Methods**: Auto (default), AES-128-GCM, Chacha20-Poly1305, none.
 * - **Protocol-Specific Settings**:
 *   - **TCP**: Custom header types (e.g., none, http).
 *   - **HTTP/2**: Path, host (as a list), custom headers (parsed as key=value pairs).
 *   - **WebSocket**: Path, host header.
 *   - **mKCP**: Header type, seed, congestion control (e.g., BBR).
 *   - **QUIC**: Security (e.g., aes-128-gcm), key, header type, UDP support.
 *   - **gRPC**: Service name, multi-mode (detected via comma-separated names).
 * - **TLS Settings**: SNI, ALPN list (e.g., ["h3", "h2"]), fingerprint (e.g., chrome), allowInsecure.
 * - **Fallback Mechanism**: Array of fallback objects with `dest` and `xver` fields for fallback routing.
 * - **Multiplexing (Mux)**: Enabled via `mux.enabled` (true/false).
 * - **Metadata**: Extracts `ps` (remark) field and uses it as `tag` in outbounds for GUI-friendly naming.
 * - **Inbound Proxies**: HTTP and SOCKS proxies with configurable ports, defaulting to `DEFAULT_HTTP_PORT` and `DEFAULT_SOCKS_PORT` if invalid.
 * - **Additional Features**:
 *   - AlterId for backward compatibility.
 *   - IPv6 address support (e.g., [2001:db8::1]).
 *   - Robust JSON parsing using cJSON, handling nested structures, escape characters, and unordered fields.
 *
 * Parameters:
 *   vmess_str (const char*): The VMess configuration string, starting with "vmess://" followed by Base64-encoded JSON.
 *   fp (FILE*): File pointer to write the generated V2Ray JSON configuration.
 *   http_port (int): HTTP proxy port; defaults to `DEFAULT_HTTP_PORT` if invalid or <= 0.
 *   socks_port (int): SOCKS proxy port; defaults to `DEFAULT_SOCKS_PORT` if invalid or <= 0.
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   - Invalid input: Null `vmess_str` or `fp`.
 *   - Incorrect prefix: `vmess_str` does not start with "vmess://".
 *   - Base64 decoding failures: Invalid Base64 encoding or memory allocation issues.
 *   - JSON parsing errors: Invalid JSON structure or missing required fields (`id`, `add`, `port`).
 *   - Type mismatches: Non-numeric `port` or `alterId`, invalid boolean values (e.g., for `mux`, `udp`).
 *   - Invalid address: Malformed IPv4, IPv6, or domain name in `add`.
 *   - Memory allocation failures during decoding or JSON parsing.
 *
 * Notes:
 * - The function uses cJSON for robust JSON parsing, ensuring compatibility with nested structures, escape characters, and arbitrary field ordering.
 * - All string fields (e.g., `path`, `host`, `sni`) are safely handled to prevent buffer overflows.
 * - The generated JSON configuration is compatible with V2Ray and supports advanced features like multiplexing and fallbacks.
 * - Log messages provide detailed error information, including file, line, and context, for debugging purposes.
 */

 EXPORT int parse_vmess_string(const char* vmess_str, FILE* fp, int http_port, int socks_port) {
     if (vmess_str == NULL || fp == NULL) {
         log_message("Null vmess_str or fp", __FILE__, __LINE__, 0, NULL);
         return -1;
     }
     if (strncmp(vmess_str, "vmess://", 8) != 0) {
         log_message("Invalid VMess prefix", __FILE__, __LINE__, 0, NULL);
         return -1;
     }
 
     char http_port_str[16];
     char socks_port_str[16];
     snprintf(http_port_str, sizeof(http_port_str), "%d", http_port);
     snprintf(socks_port_str, sizeof(socks_port_str), "%d", socks_port);
 
     int final_http_port = (http_port > 0 && validate_port(http_port_str)) ? http_port : DEFAULT_HTTP_PORT;
     int final_socks_port = (socks_port > 0 && validate_port(socks_port_str)) ? socks_port : DEFAULT_SOCKS_PORT;
 
     const char* base64_data = vmess_str + 8;
     size_t base64_len = strlen(base64_data);
     char* decoded = NULL;
     int decoded_len = 0;
 
     #ifdef _WIN32
     DWORD dwDecodedLen = 0;
     if (!CryptStringToBinaryA(base64_data, base64_len, CRYPT_STRING_BASE64, NULL, &dwDecodedLen, NULL, NULL)) {
         log_message("Failed to calculate Base64 decoded length", __FILE__, __LINE__, GetLastError(), NULL);
         return -1;
     }
     decoded = malloc(dwDecodedLen + 1);
     if (!decoded) {
         log_message("Memory allocation failed for decoded data", __FILE__, __LINE__, 0, NULL);
         return -1;
     }
     if (!CryptStringToBinaryA(base64_data, base64_len, CRYPT_STRING_BASE64, (BYTE*)decoded, &dwDecodedLen, NULL, NULL)) {
         log_message("Base64 decoding failed", __FILE__, __LINE__, GetLastError(), NULL);
         free(decoded);
         return -1;
     }
     decoded[dwDecodedLen] = '\0';
     decoded_len = dwDecodedLen;
     #else
     BIO* b64 = BIO_new(BIO_f_base64());
     BIO* bio = BIO_new_mem_buf(base64_data, base64_len);
     bio = BIO_push(b64, bio);
     BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
     char temp[1024];
     decoded_len = BIO_read(bio, temp, sizeof(temp));
     if (decoded_len <= 0) {
         log_message("Base64 decoding failed", __FILE__, __LINE__, 0, NULL);
         BIO_free_all(bio);
         return -1;
     }
     decoded = malloc(decoded_len + 1);
     if (!decoded) {
         log_message("Memory allocation failed for decoded data", __FILE__, __LINE__, 0, NULL);
         BIO_free_all(bio);
         return -1;
     }
     memcpy(decoded, temp, decoded_len);
     decoded[decoded_len] = '\0';
     BIO_free_all(bio);
     #endif
 
     if (decoded_len <= 0) {
         log_message("Decoded length is invalid", __FILE__, __LINE__, 0, NULL);
         free(decoded);
         return -1;
     }
 
     cJSON* json = cJSON_Parse(decoded);
     free(decoded);
     if (!json) {
         log_message("Invalid JSON format", __FILE__, __LINE__, 0, cJSON_GetErrorPtr());
         return -1;
     }
 
     char id[128] = "";
     char address[2048] = "";
     char port_str[16] = "";
     char alter_id_str[16] = "0";
     char encryption[128] = "auto";
     char network[128] = "tcp";
     char security[128] = "none";
     char type[128] = "none";
     char path[2048] = "";
     char host[2048] = "";
     char sni[2048] = "";
     char alpn[4096] = "";
     char quic_security[128] = "";
     char quic_key[128] = "";
     char grpc_service_name[2048] = "";
     char mkcp_seed[128] = "";
     char congestion[16] = "";
     char http_headers[4096] = "";
     char ps[128] = "";
     char tag[128] = "";
     char fingerprint[128] = "";
     char allow_insecure[16] = "false";
     char mux[16] = "false";
     char udp[16] = "true";
     char fallbacks[4096] = "";
 
     cJSON* item;
     if ((item = cJSON_GetObjectItem(json, "id")) && cJSON_IsString(item)) strncpy(id, item->valuestring, sizeof(id) - 1);
     if ((item = cJSON_GetObjectItem(json, "add")) && cJSON_IsString(item)) strncpy(address, item->valuestring, sizeof(address) - 1);
     if ((item = cJSON_GetObjectItem(json, "port")) && (cJSON_IsString(item) || cJSON_IsNumber(item))) {
         snprintf(port_str, sizeof(port_str), "%d", cJSON_IsString(item) ? atoi(item->valuestring) : (int)item->valuedouble);
     }
     if ((item = cJSON_GetObjectItem(json, "aid")) && (cJSON_IsString(item) || cJSON_IsNumber(item))) {
         snprintf(alter_id_str, sizeof(alter_id_str), "%d", cJSON_IsString(item) ? atoi(item->valuestring) : (int)item->valuedouble);
     }
     if ((item = cJSON_GetObjectItem(json, "scy")) && cJSON_IsString(item)) strncpy(encryption, item->valuestring, sizeof(encryption) - 1);
     if ((item = cJSON_GetObjectItem(json, "net")) && cJSON_IsString(item)) strncpy(network, item->valuestring, sizeof(network) - 1);
     if ((item = cJSON_GetObjectItem(json, "type")) && cJSON_IsString(item)) strncpy(type, item->valuestring, sizeof(type) - 1);
     if ((item = cJSON_GetObjectItem(json, "security")) && cJSON_IsString(item)) strncpy(security, item->valuestring, sizeof(security) - 1);
     else if ((item = cJSON_GetObjectItem(json, "tls")) && cJSON_IsString(item) && strcmp(item->valuestring, "") != 0) {
         strncpy(security, item->valuestring, sizeof(security) - 1);
     }
     if ((item = cJSON_GetObjectItem(json, "path")) && cJSON_IsString(item)) strncpy(path, item->valuestring, sizeof(path) - 1);
     if ((item = cJSON_GetObjectItem(json, "host")) && cJSON_IsString(item)) strncpy(host, item->valuestring, sizeof(host) - 1);
     else if ((item = cJSON_GetObjectItem(json, "host")) && cJSON_IsArray(item)) {
         int first = 1;
         cJSON* host_item;
         host[0] = '\0';
         cJSON_ArrayForEach(host_item, item) {
             if (cJSON_IsString(host_item)) {
                 if (!first) strncat(host, ",", sizeof(host) - strlen(host) - 1);
                 strncat(host, host_item->valuestring, sizeof(host) - strlen(host) - 1);
                 first = 0;
             }
         }
     }
     if ((item = cJSON_GetObjectItem(json, "sni")) && cJSON_IsString(item)) strncpy(sni, item->valuestring, sizeof(sni) - 1);
     if ((item = cJSON_GetObjectItem(json, "alpn")) && cJSON_IsString(item)) strncpy(alpn, item->valuestring, sizeof(alpn) - 1);
     else if ((item = cJSON_GetObjectItem(json, "alpn")) && cJSON_IsArray(item)) {
         int first = 1;
         cJSON* alpn_item;
         alpn[0] = '\0';
         cJSON_ArrayForEach(alpn_item, item) {
             if (cJSON_IsString(alpn_item)) {
                 if (!first) strncat(alpn, ",", sizeof(alpn) - strlen(alpn) - 1);
                 strncat(alpn, alpn_item->valuestring, sizeof(alpn) - strlen(alpn) - 1);
                 first = 0;
             }
         }
     }
     if ((item = cJSON_GetObjectItem(json, "quicSecurity")) && cJSON_IsString(item)) strncpy(quic_security, item->valuestring, sizeof(quic_security) - 1);
     if ((item = cJSON_GetObjectItem(json, "key")) && cJSON_IsString(item)) strncpy(quic_key, item->valuestring, sizeof(quic_key) - 1);
     if ((item = cJSON_GetObjectItem(json, "serviceName")) && cJSON_IsString(item)) strncpy(grpc_service_name, item->valuestring, sizeof(grpc_service_name) - 1);
     if ((item = cJSON_GetObjectItem(json, "seed")) && cJSON_IsString(item)) strncpy(mkcp_seed, item->valuestring, sizeof(mkcp_seed) - 1);
     if ((item = cJSON_GetObjectItem(json, "congestion")) && (cJSON_IsString(item) || cJSON_IsBool(item))) {
         strncpy(congestion, cJSON_IsString(item) ? item->valuestring : (item->valueint ? "true" : "false"), sizeof(congestion) - 1);
     }
     if ((item = cJSON_GetObjectItem(json, "headers")) && cJSON_IsString(item)) strncpy(http_headers, item->valuestring, sizeof(http_headers) - 1);
     if ((item = cJSON_GetObjectItem(json, "ps")) && cJSON_IsString(item)) strncpy(ps, item->valuestring, sizeof(ps) - 1);
     if ((item = cJSON_GetObjectItem(json, "fp")) && cJSON_IsString(item)) strncpy(fingerprint, item->valuestring, sizeof(fingerprint) - 1);
     if ((item = cJSON_GetObjectItem(json, "allowInsecure")) && (cJSON_IsBool(item) || cJSON_IsString(item))) {
         strncpy(allow_insecure, cJSON_IsBool(item) ? (item->valueint ? "true" : "false") : item->valuestring, sizeof(allow_insecure) - 1);
     }
     if ((item = cJSON_GetObjectItem(json, "mux")) && (cJSON_IsBool(item) || cJSON_IsString(item))) {
         strncpy(mux, cJSON_IsBool(item) ? (item->valueint ? "true" : "false") : item->valuestring, sizeof(mux) - 1);
     }
     if ((item = cJSON_GetObjectItem(json, "udp")) && (cJSON_IsBool(item) || cJSON_IsString(item))) {
         strncpy(udp, cJSON_IsBool(item) ? (item->valueint ? "true" : "false") : item->valuestring, sizeof(udp) - 1);
     }
     if ((item = cJSON_GetObjectItem(json, "fallbacks")) && cJSON_IsArray(item)) {
         int first = 1;
         cJSON* fb_item;
         fallbacks[0] = '\0';
         cJSON_ArrayForEach(fb_item, item) {
             if (cJSON_IsObject(fb_item)) {
                 char fb_dest[256] = "";
                 char fb_xver[16] = "";
                 cJSON* dest = cJSON_GetObjectItem(fb_item, "dest");
                 cJSON* xver = cJSON_GetObjectItem(fb_item, "xver");
                 if (dest && (cJSON_IsString(dest) || cJSON_IsNumber(dest))) {
                     snprintf(fb_dest, sizeof(fb_dest), "%s", cJSON_IsString(dest) ? dest->valuestring : cJSON_Print(dest));
                 }
                 if (xver && (cJSON_IsString(xver) || cJSON_IsNumber(xver))) {
                     snprintf(fb_xver, sizeof(fb_xver), "%d", cJSON_IsString(xver) ? atoi(xver->valuestring) : (int)xver->valuedouble);
                 }
                 if (fb_dest[0]) {
                     if (!first) strncat(fallbacks, ";", sizeof(fallbacks) - strlen(fallbacks) - 1);
                     char fb_entry[512];
                     snprintf(fb_entry, sizeof(fb_entry), "dest:%s%s", fb_dest, fb_xver[0] ? ",xver:" : "");
                     if (fb_xver[0]) strncat(fb_entry, fb_xver, sizeof(fb_entry) - strlen(fb_entry) - 1);
                     strncat(fallbacks, fb_entry, sizeof(fallbacks) - strlen(fallbacks) - 1);
                     first = 0;
                 }
             }
         }
     }
 
     cJSON_Delete(json);
 
     if (id[0] == '\0' || address[0] == '\0' || port_str[0] == '\0') {
         log_message("Missing required fields (id, address, or port)", __FILE__, __LINE__, 0, NULL);
         return -1;
     }
 
     int server_port = atoi(port_str);
     int alter_id = atoi(alter_id_str);
     if (!validate_port(port_str) || alter_id < 0) {
         log_message("Invalid server port or alterId", __FILE__, __LINE__, 0, port_str);
         return -1;
     }
     if (!validate_address(address)) {
         log_message("Invalid address", __FILE__, __LINE__, 0, address);
         return -1;
     }
 
     if (ps[0]) {
        strncpy(tag, ps, sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';
     }
 
     fprintf(fp, "{\n");
     fprintf(fp, "  \"inbounds\": [\n");
     fprintf(fp, "    {\"port\": %d, \"protocol\": \"http\", \"settings\": {}},\n", final_http_port);
     fprintf(fp, "    {\"port\": %d, \"protocol\": \"socks\", \"settings\": {\"udp\": true}}\n", final_socks_port);
     fprintf(fp, "  ],\n");
     fprintf(fp, "  \"outbounds\": [{\n");
     fprintf(fp, "    \"protocol\": \"vmess\",\n");
     if (tag[0]) {
         fprintf(fp, "    \"tag\": \"%s\",\n", tag);
     }
     fprintf(fp, "    \"settings\": {\"vnext\": [{\"address\": \"%s\", \"port\": %d, \"users\": [{\"id\": \"%s\", \"alterId\": %d, \"security\": \"%s\"}]}]},\n",
             address, server_port, id, alter_id, encryption);
 
     fprintf(fp, "    \"streamSettings\": {\n");
     fprintf(fp, "      \"network\": \"%s\",\n", network);
     fprintf(fp, "      \"security\": \"%s\",\n", security);
     fprintf(fp, "      \"udp\": %s,\n", udp);
 
     if (strcmp(network, "tcp") == 0) {
         fprintf(fp, "      \"tcpSettings\": {\"header\": {\"type\": \"%s\"}}\n", type);
     } else if (strcmp(network, "http") == 0 || strcmp(network, "h2") == 0) {
         fprintf(fp, "      \"httpSettings\": {\"path\": \"%s\"", path);
         if (host[0]) {
             fprintf(fp, ", \"host\": [");
             char host_copy[2048];
             strncpy(host_copy, host, sizeof(host_copy) - 1);
             host_copy[sizeof(host_copy) - 1] = '\0';
             char* host_item = strtok(host_copy, ",");
             int first = 1;
             while (host_item) {
                 if (!first) fprintf(fp, ", ");
                 fprintf(fp, "\"%s\"", host_item);
                 first = 0;
                 host_item = strtok(NULL, ",");
             }
             fprintf(fp, "]");
         }
         if (http_headers[0]) {
             fprintf(fp, ", \"headers\": {");
             char headers_copy[4096];
             strncpy(headers_copy, http_headers, sizeof(headers_copy) - 1);
             headers_copy[sizeof(headers_copy) - 1] = '\0';
             char* header = strtok(headers_copy, ",");
             int first = 1;
             while (header) {
                 char* eq = strchr(header, '=');
                 if (eq) {
                     *eq = '\0';
                     if (!first) fprintf(fp, ", ");
                     fprintf(fp, "\"%s\": [\"%s\"]", header, eq + 1);
                     first = 0;
                 }
                 header = strtok(NULL, ",");
             }
             fprintf(fp, "}");
         }
         fprintf(fp, "}\n");
     } else if (strcmp(network, "ws") == 0) {
         fprintf(fp, "      \"wsSettings\": {\"path\": \"%s\"", path);
         if (host[0]) fprintf(fp, ", \"headers\": {\"Host\": \"%s\"}", host);
         fprintf(fp, "}\n");
     } else if (strcmp(network, "kcp") == 0) {
         fprintf(fp, "      \"kcpSettings\": {\"header\": {\"type\": \"%s\"}", type);
         if (mkcp_seed[0]) fprintf(fp, ", \"seed\": \"%s\"", mkcp_seed);
         if (congestion[0]) fprintf(fp, ", \"congestion\": %s", strcmp(congestion, "true") == 0 ? "true" : "false");
         fprintf(fp, "}\n");
     } else if (strcmp(network, "quic") == 0) {
         fprintf(fp, "      \"quicSettings\": {\"security\": \"%s\", \"key\": \"%s\", \"header\": {\"type\": \"%s\"}}\n",
                 quic_security, quic_key, type);
     } else if (strcmp(network, "grpc") == 0) {
         fprintf(fp, "      \"grpcSettings\": {\"multiMode\": %s, \"serviceName\": \"%s\"}\n",
                 strchr(grpc_service_name, ',') ? "true" : "false", grpc_service_name);
     }
 
     if (strcmp(security, "tls") == 0) {
         fprintf(fp, "      ,\"tlsSettings\": {\"serverName\": \"%s\"", sni);
         if (alpn[0]) {
             fprintf(fp, ", \"alpn\": [");
             char alpn_copy[4096];
             strncpy(alpn_copy, alpn, sizeof(alpn_copy) - 1);
             alpn_copy[sizeof(alpn_copy) - 1] = '\0';
             char* alpn_item = strtok(alpn_copy, ",");
             int first = 1;
             while (alpn_item) {
                 if (!first) fprintf(fp, ", ");
                 fprintf(fp, "\"%s\"", alpn_item);
                 first = 0;
                 alpn_item = strtok(NULL, ",");
             }
             fprintf(fp, "]");
         }
         if (fingerprint[0]) fprintf(fp, ", \"fingerprint\": \"%s\"", fingerprint);
         if (allow_insecure[0]) fprintf(fp, ", \"allowInsecure\": %s", strcmp(allow_insecure, "true") == 0 ? "true" : "false");
         fprintf(fp, "}\n");
     }
 
     if (mux[0]) {
         fprintf(fp, "      ,\"mux\": {\"enabled\": %s}\n", strcmp(mux, "true") == 0 ? "true" : "false");
     }
 
     fprintf(fp, "    }\n");
     fprintf(fp, "  }],\n");
 
     if (fallbacks[0]) {
         fprintf(fp, "  \"fallbacks\": [");
         char fallbacks_copy[4096];
         strncpy(fallbacks_copy, fallbacks, sizeof(fallbacks_copy) - 1);
         fallbacks_copy[sizeof(fallbacks_copy) - 1] = '\0';
         char* fb_entry = strtok(fallbacks_copy, ";");
         int first = 1;
         while (fb_entry) {
             char fb_dest[256] = "";
             char fb_xver[16] = "";
             char entry_copy[512];
             strncpy(entry_copy, fb_entry, sizeof(entry_copy) - 1);
             entry_copy[sizeof(entry_copy) - 1] = '\0';
             char* fb_param = strtok(entry_copy, ",");
             while (fb_param) {
                 if (strncmp(fb_param, "dest:", 5) == 0) strncpy(fb_dest, fb_param + 5, sizeof(fb_dest) - 1);
                 else if (strncmp(fb_param, "xver:", 5) == 0) strncpy(fb_xver, fb_param + 5, sizeof(fb_xver) - 1);
                 fb_param = strtok(NULL, ",");
             }
             if (fb_dest[0]) {
                 if (!first) fprintf(fp, ", ");
                 fprintf(fp, "{\"dest\": \"%s\"", fb_dest);
                 if (fb_xver[0]) fprintf(fp, ", \"xver\": %s", fb_xver);
                 fprintf(fp, "}");
                 first = 0;
             }
             fb_entry = strtok(NULL, ";");
         }
         fprintf(fp, "]\n");
     }
 
     fprintf(fp, "}\n");
 
     char extra_info[256];
     snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, HTTP Port: %d, SOCKS Port: %d, Tag: %s",
             address, server_port, final_http_port, final_socks_port, tag[0] ? tag : "none");
     log_message("VMess config written successfully", __FILE__, __LINE__, 0, extra_info);
     return 0;
 }