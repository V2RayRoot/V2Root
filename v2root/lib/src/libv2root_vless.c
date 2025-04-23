 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "libv2root_vless.h"
 #include "libv2root_core.h"
 #include "libv2root_utils.h"
/*
 * Parses a VLESS configuration string and writes the resulting JSON configuration to a file.
 *
 * Supports the VLESS protocol with various transport and security options, including:
 * - Transport protocols: TCP, HTTP/2 (h2), WebSocket (ws), mKCP, QUIC, gRPC
 * - Security options: None, TLS, Reality
 * - Encryption: None (default)
 * - Flow control: Optional flow parameter for advanced routing
 * - TCP settings: Custom header types (e.g., none, http)
 * - HTTP/2 settings: Path, host, custom headers
 * - WebSocket settings: Path, host header
 * - mKCP settings: Header type, seed, congestion control (e.g., BBR)
 * - QUIC settings: Security, key, header type
 * - gRPC settings: Service name, multi-mode support
 * - TLS settings: Server Name Indication (SNI), ALPN, fingerprint
 * - Reality settings: Public key, short IDs, spiderX, fingerprint
 * - Inbound proxies: HTTP and SOCKS with configurable ports
 *
 * Parameters:
 *   vless_str (const char*): The VLESS configuration string (e.g., vless://uuid@address:port?params).
 *   fp (FILE*): File pointer to write the JSON configuration.
 *   http_port (int): The HTTP proxy port (defaults to DEFAULT_HTTP_PORT if invalid or <= 0).
 *   socks_port (int): The SOCKS proxy port (defaults to DEFAULT_SOCKS_PORT if invalid or <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   Logs errors for invalid input, incorrect VLESS prefix, parsing failures, invalid port/address,
 *   or parameter buffer overflow.
 */
 
 EXPORT int parse_vless_string(const char* vless_str, FILE* fp, int http_port, int socks_port) {
    if (vless_str == NULL || fp == NULL) {
        log_message("Null vless_str or fp", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    if (strncmp(vless_str, "vless://", 8) != 0) {
        log_message("Invalid VLESS prefix", __FILE__, __LINE__, 0, vless_str);
        return -1;
    }

    char http_port_str[16];
    char socks_port_str[16];
    snprintf(http_port_str, sizeof(http_port_str), "%d", http_port);
    snprintf(socks_port_str, sizeof(socks_port_str), "%d", socks_port);

    int final_http_port = (http_port > 0 && validate_port(http_port_str)) ? http_port : DEFAULT_HTTP_PORT;
    int final_socks_port = (socks_port > 0 && validate_port(socks_port_str)) ? socks_port : DEFAULT_SOCKS_PORT;

    char uuid[128] = "";
    char address[2048] = "";
    char port_str[16] = "";
    char params[4096] = "";

    if (sscanf(vless_str, "vless://%127[^@]@%2047[^:]:%15[^?]?%4095s", uuid, address, port_str, params) != 4) {
        log_message("Failed to parse VLESS format", __FILE__, __LINE__, 0, vless_str);
        return -1;
    }

    int server_port = atoi(port_str);
    if (!validate_port(port_str)) {
        log_message("Invalid server port", __FILE__, __LINE__, 0, port_str);
        return -1;
    }
    if (!validate_address(address)) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Address validation failed for: %s", address);
        log_message(err_msg, __FILE__, __LINE__, 0, vless_str);
        return -1;
    }

    // بقیه پارامترها مثل قبل
    char encryption[128] = "none";
    char flow[128] = "";
    char network[128] = "tcp";
    char security[128] = "none";
    char header_type[128] = "none";
    char path[2048] = "";
    char host[2048] = "";
    char sni[2048] = "";
    char alpn[128] = "";
    char fingerprint[128] = "";
    char public_key[2048] = "";
    char short_ids[2048] = "";
    char spider_x[2048] = "";
    char quic_security[128] = "";
    char quic_key[128] = "";
    char grpc_service_name[2048] = "";
    char mkcp_seed[128] = "";
    char congestion[16] = "";
    char http_headers[4096] = "";

    char params_copy[4096];
    if (strlen(params) >= sizeof(params_copy)) {
        log_message("Parameters exceed buffer size", __FILE__, __LINE__, 0, params);
        return -1;
    }
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
        else if (strncmp(param, "sni=", 4) == 0) strncpy(sni, param + 4, sizeof(sni) - 1);
        else if (strncmp(param, "alpn=", 5) == 0) strncpy(alpn, param + 5, sizeof(alpn) - 1);
        else if (strncmp(param, "fp=", 3) == 0) strncpy(fingerprint, param + 3, sizeof(fingerprint) - 1);
        else if (strncmp(param, "pbk=", 4) == 0) strncpy(public_key, param + 4, sizeof(public_key) - 1);
        else if (strncmp(param, "sid=", 4) == 0) strncpy(short_ids, param + 4, sizeof(short_ids) - 1);
        else if (strncmp(param, "spx=", 4) == 0) strncpy(spider_x, param + 4, sizeof(spider_x) - 1);
        else if (strncmp(param, "quicSecurity=", 13) == 0) strncpy(quic_security, param + 13, sizeof(quic_security) - 1);
        else if (strncmp(param, "key=", 4) == 0) strncpy(quic_key, param + 4, sizeof(quic_key) - 1);
        else if (strncmp(param, "serviceName=", 12) == 0) strncpy(grpc_service_name, param + 12, sizeof(grpc_service_name) - 1);
        else if (strncmp(param, "seed=", 5) == 0) strncpy(mkcp_seed, param + 5, sizeof(mkcp_seed) - 1);
        else if (strncmp(param, "congestion=", 11) == 0) strncpy(congestion, param + 11, sizeof(congestion) - 1);
        else if (strncmp(param, "headers=", 8) == 0) strncpy(http_headers, param + 8, sizeof(http_headers) - 1);
        param = strtok(NULL, "&");
    }

    // نوشتن کانفیگ توی فایل
    fprintf(fp, "{\n");
    fprintf(fp, "  \"inbounds\": [\n");
    fprintf(fp, "    {\"port\": %d, \"protocol\": \"http\", \"settings\": {}},\n", final_http_port);
    fprintf(fp, "    {\"port\": %d, \"protocol\": \"socks\", \"settings\": {\"udp\": true}}\n", final_socks_port);
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"outbounds\": [{\n");
    fprintf(fp, "    \"protocol\": \"vless\",\n");
    fprintf(fp, "    \"settings\": {\"vnext\": [{\"address\": \"%s\", \"port\": %d, \"users\": [{\"id\": \"%s\", \"encryption\": \"%s\"",
            address, server_port, uuid, encryption);
    if (flow[0]) fprintf(fp, ", \"flow\": \"%s\"", flow);
    fprintf(fp, "}]}]},\n");

    fprintf(fp, "    \"streamSettings\": {\n");
    fprintf(fp, "      \"network\": \"%s\",\n", network);
    fprintf(fp, "      \"security\": \"%s\",\n", security);

    if (strcmp(network, "tcp") == 0) {
        fprintf(fp, "      \"tcpSettings\": {\"header\": {\"type\": \"%s\"}}\n", header_type);
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
        fprintf(fp, "      \"kcpSettings\": {\"header\": {\"type\": \"%s\"}", header_type);
        if (mkcp_seed[0]) fprintf(fp, ", \"seed\": \"%s\"", mkcp_seed);
        if (congestion[0]) fprintf(fp, ", \"congestion\": %s", strcmp(congestion, "bbr") == 0 ? "true" : "false");
        fprintf(fp, "}\n");
    } else if (strcmp(network, "quic") == 0) {
        fprintf(fp, "      \"quicSettings\": {\"security\": \"%s\", \"key\": \"%s\", \"header\": {\"type\": \"%s\"}}\n",
                quic_security, quic_key, header_type);
    } else if (strcmp(network, "grpc") == 0) {
        fprintf(fp, "      \"grpcSettings\": {\"multiMode\": %s, \"serviceName\": \"%s\"}\n",
                strchr(grpc_service_name, ',') ? "true" : "false", grpc_service_name);
    }

    if (strcmp(security, "tls") == 0) {
        fprintf(fp, "      ,\"tlsSettings\": {\"serverName\": \"%s\"", sni);
        if (alpn[0]) fprintf(fp, ", \"alpn\": [\"%s\"]", alpn);
        if (fingerprint[0]) fprintf(fp, ", \"fingerprint\": \"%s\"", fingerprint);
        fprintf(fp, "}\n");
    } else if (strcmp(security, "reality") == 0) {
        fprintf(fp, "      ,\"realitySettings\": {\"publicKey\": \"%s\"", public_key);
        if (short_ids[0]) fprintf(fp, ", \"shortIds\": [\"%s\"]", short_ids);
        if (spider_x[0]) fprintf(fp, ", \"spiderX\": \"%s\"", spider_x);
        if (fingerprint[0]) fprintf(fp, ", \"fingerprint\": \"%s\"", fingerprint);
        fprintf(fp, "}\n");
    }

    fprintf(fp, "    }\n");
    fprintf(fp, "  }]\n");
    fprintf(fp, "}\n");

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, HTTP Port: %d, SOCKS Port: %d",
             address, server_port, final_http_port, final_socks_port);
    log_message("VLESS config written successfully", __FILE__, __LINE__, 0, extra_info);
    return 0;
}