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
#include "libv2root_vmess.h"
#include "libv2root_core.h"
#include "libv2root_utils.h"

/*
 * Parses a VMess configuration string and writes the resulting JSON configuration to a file.
 *
 * Supports the VMess protocol with comprehensive transport and security options, including:
 * - Transport protocols: TCP, HTTP/2 (h2), WebSocket (ws), mKCP, QUIC, gRPC
 * - Security options: None, TLS
 * - Encryption methods: Auto (default), AES-128-GCM, Chacha20-Poly1305, none
 * - TCP settings: Custom header types (e.g., none, http)
 * - HTTP/2 settings: Path, host, custom headers
 * - WebSocket settings: Path, host header
 * - mKCP settings: Header type, seed, congestion control (e.g., BBR)
 * - QUIC settings: Security, key, header type
 * - gRPC settings: Service name, multi-mode support
 * - TLS settings: Server Name Indication (SNI), ALPN
 * - Inbound proxies: HTTP and SOCKS with configurable ports
 * - Additional features: AlterId for backward compatibility
 *
 * Parameters:
 *   vmess_str (const char*): The VMess configuration string (e.g., vmess://base64_encoded_json).
 *   fp (FILE*): File pointer to write the JSON configuration.
 *   http_port (int): The HTTP proxy port (defaults to DEFAULT_HTTP_PORT if invalid or <= 0).
 *   socks_port (int): The SOCKS proxy port (defaults to DEFAULT_SOCKS_PORT if invalid or <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for invalid input, incorrect VMess prefix, Base64 decoding failures, missing required fields,
 *   invalid port/address, or memory allocation failures.
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
        
    char id[128] = "";
    char address[2048] = "";
    char port_str[16] = "";
    char alter_id_str[16] = "0";
    char security[128] = "none";
    char encryption[128] = "auto";
    char network[128] = "tcp";
    char type[128] = "none";
    char path[2048] = "";
    char host[2048] = "";
    char sni[2048] = "";
    char alpn[128] = "";
    char quic_security[128] = "";
    char quic_key[128] = "";
    char grpc_service_name[2048] = "";
    char mkcp_seed[128] = "";
    char congestion[16] = "";
    char http_headers[4096] = "";

    char* ptr = decoded;
    char* key_start;
    char* value_start;
    char* value_end;

    while (*ptr) {
        key_start = strchr(ptr, '"');
        if (!key_start) break;
        ptr = key_start + 1;

        value_start = strchr(ptr, ':');
        if (!value_start) break;
        *value_start = '\0';
        value_start++;

        while (*value_start == ' ' || *value_start == '\t') value_start++;

        if (*value_start == '"') {
            value_start++;
            value_end = strchr(value_start, '"');
            if (!value_end) break;
            *value_end = '\0';
        } else {
            value_end = strchr(value_start, ',');
            if (!value_end) value_end = strchr(value_start, '}');
            if (!value_end) break;
            *value_end = '\0';
            while (value_end > value_start && (*(value_end - 1) == ' ' || *(value_end - 1) == '\t')) value_end--;
        }

        if (strcmp(key_start, "\"id\"") == 0) strncpy(id, value_start, sizeof(id) - 1);
        else if (strcmp(key_start, "\"add\"") == 0) strncpy(address, value_start, sizeof(address) - 1);
        else if (strcmp(key_start, "\"port\"") == 0) strncpy(port_str, value_start, sizeof(port_str) - 1);
        else if (strcmp(key_start, "\"aid\"") == 0) strncpy(alter_id_str, value_start, sizeof(alter_id_str) - 1);
        else if (strcmp(key_start, "\"scy\"") == 0) strncpy(encryption, value_start, sizeof(encryption) - 1);
        else if (strcmp(key_start, "\"net\"") == 0) strncpy(network, value_start, sizeof(network) - 1);
        else if (strcmp(key_start, "\"type\"") == 0) strncpy(type, value_start, sizeof(type) - 1);
        else if (strcmp(key_start, "\"security\"") == 0 || strcmp(key_start, "\"tls\"") == 0) {
            if (*value_start) strncpy(security, value_start, sizeof(security) - 1);
        }
        else if (strcmp(key_start, "\"path\"") == 0) strncpy(path, value_start, sizeof(path) - 1);
        else if (strcmp(key_start, "\"host\"") == 0) strncpy(host, value_start, sizeof(host) - 1);
        else if (strcmp(key_start, "\"sni\"") == 0) strncpy(sni, value_start, sizeof(sni) - 1);
        else if (strcmp(key_start, "\"alpn\"") == 0) strncpy(alpn, value_start, sizeof(alpn) - 1);
        else if (strcmp(key_start, "\"quicSecurity\"") == 0) strncpy(quic_security, value_start, sizeof(quic_security) - 1);
        else if (strcmp(key_start, "\"key\"") == 0) strncpy(quic_key, value_start, sizeof(quic_key) - 1);
        else if (strcmp(key_start, "\"serviceName\"") == 0) strncpy(grpc_service_name, value_start, sizeof(grpc_service_name) - 1);
        else if (strcmp(key_start, "\"seed\"") == 0) strncpy(mkcp_seed, value_start, sizeof(mkcp_seed) - 1);
        else if (strcmp(key_start, "\"congestion\"") == 0) strncpy(congestion, value_start, sizeof(congestion) - 1);
        else if (strcmp(key_start, "\"headers\"") == 0) strncpy(http_headers, value_start, sizeof(http_headers) - 1);

        ptr = value_end + 1;
        while (*ptr == ' ' || *ptr == '\t' || *ptr == ',') ptr++;
    }

    free(decoded);

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

    fprintf(fp, "{\n");
    fprintf(fp, "  \"inbounds\": [\n");
    fprintf(fp, "    {\"port\": %d, \"protocol\": \"http\", \"settings\": {}},\n", final_http_port);
    fprintf(fp, "    {\"port\": %d, \"protocol\": \"socks\", \"settings\": {\"udp\": true}}\n", final_socks_port);
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"outbounds\": [{\n");
    fprintf(fp, "    \"protocol\": \"vmess\",\n");
    fprintf(fp, "    \"settings\": {\"vnext\": [{\"address\": \"%s\", \"port\": %d, \"users\": [{\"id\": \"%s\", \"alterId\": %d, \"security\": \"%s\"}]}]},\n",
            address, server_port, id, alter_id, encryption);

    fprintf(fp, "    \"streamSettings\": {\n");
    fprintf(fp, "      \"network\": \"%s\",\n", network);
    fprintf(fp, "      \"security\": \"%s\",\n", security);

    if (strcmp(network, "tcp") == 0) {
        fprintf(fp, "      \"tcpSettings\": {\"header\": {\"type\": \"%s\"}}\n", type);
    } else if (strcmp(network, "http") == 0 || strcmp(network, "h2") == 0) {
        fprintf(fp, "      \"httpSettings\": {\"path\": \"%s\"", path);
        if (host[0]) fprintf(fp, ", \"host\": [\"%s\"]", host);
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
        if (congestion[0]) fprintf(fp, ", \"congestion\": %s", strcmp(congestion, "bbr") == 0 ? "true" : "false");
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
        if (alpn[0]) fprintf(fp, ", \"alpn\": [\"%s\"]", alpn);
        fprintf(fp, "}\n");
    }

    fprintf(fp, "    }\n");
    fprintf(fp, "  }]\n");
    fprintf(fp, "}\n");

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, HTTP Port: %d, SOCKS Port: %d",
            address, server_port, final_http_port, final_socks_port);
    log_message("VMess config written successfully", __FILE__, __LINE__, 0, extra_info);
    return 0;
}