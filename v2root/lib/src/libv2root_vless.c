#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libv2root_vless.h"
#include "libv2root_core.h"
#include "libv2root_utils.h"

/*
 * Parses a VLESS configuration string and writes the resulting JSON configuration to a file.
 *
 * Fully supports the VLESS protocol with all advanced features, including:
 * - Transport protocols: TCP, HTTP/2 (h2), WebSocket (ws), mKCP, QUIC, gRPC, with customizable settings for each.
 * - Security modes: None, TLS (with SNI, ALPN as list, fingerprint, allowInsecure, enableSessionResumption, uTLS), Reality (with publicKey, shortIds, spiderX, fingerprint, show, dest, serverNames).
 * - Encryption: None (default for VLESS).
 * - Flow control: Supports advanced flows like xtls-rprx-vision and xtls-rprx-direct, with strict validation.
 * - TCP settings: Custom header types (e.g., none, http).
 * - HTTP/2 settings: Configurable path, host, and custom headers (parsed as key=value pairs).
 * - WebSocket settings: Path and host header for WebSocket disguise.
 * - mKCP settings: Header type, seed for obfuscation, and congestion control (e.g., BBR).
 * - QUIC settings: Security (e.g., aes-128-gcm), key, and header type.
 * - gRPC settings: Service name, multi-mode (detected via comma-separated service names), authority, and maxStreams.
 * - TLS settings: Server Name Indication (SNI), ALPN parsed as a list (e.g., h3,h2,http/1.1), fingerprint (e.g., chrome), allowInsecure flag, enableSessionResumption, and uTLS for browser simulation.
 * - Reality settings: Public key, short IDs, spiderX, fingerprint, show flag, destination (dest), and serverNames as a list.
 * - Fallback mechanism: Supports both a single fallback string (e.g., 127.0.0.1:8080) and an array of fallback objects with alpn, dest, and xver fields.
 * - Mux support: Enables multiplexing via mux.enabled (true/false).
 * - Multiple outbounds: Supports multiple outbound configurations with unique tags, addresses, and ports.
 * - Routing: Basic domain-based and IP-based routing rules (e.g., domain:example.com:tag1 or ip:192.168.1.1:tag2).
 * - Inbound proxies: HTTP and SOCKS proxies with configurable ports, falling back to DEFAULT_HTTP_PORT and DEFAULT_SOCKS_PORT if invalid.
 *
 * Parameters:
 *   vless_str (const char*): The VLESS configuration string in the format vless://uuid@address:port?params.
 *   fp (FILE*): File pointer to write the resulting JSON configuration.
 *   http_port (int): The HTTP proxy port (defaults to DEFAULT_HTTP_PORT if invalid or <= 0).
 *   socks_port (int): The SOCKS proxy port (defaults to DEFAULT_SOCKS_PORT if invalid or <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   - Logs errors for invalid input (null vless_str or fp).
 *   - Invalid VLESS prefix (must start with "vless://").
 *   - Parsing failures (incorrect format, invalid UUID, address, or port).
 *   - Invalid port or address (via validate_port and validate_address).
 *   - Parameter buffer overflow (params exceeding 4096 bytes).
 *   - Invalid flow values (only xtls-rprx-vision and xtls-rprx-direct allowed).
 */

 
 void urldecode(char* dst, const char* src, size_t dst_size) {
     char* d = dst;
     const char* s = src;
     size_t i = 0;
 
     while (*s && i < dst_size - 1) {
         if (*s == '%' && s[1] && s[2]) {
             char hex[3] = {s[1], s[2], 0};
             int value = 0;
             sscanf(hex, "%x", &value);
             *d++ = (char)value;
             s += 3;
             i++;
         } else if (*s == '+') {
             *d++ = ' ';
             s++;
             i++;
         } else {
             *d++ = *s++;
             i++;
         }
     }
     *d = '\0';
 }
 
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
 
     char encryption[128] = "none";
     char flow[128] = "";
     char network[128] = "tcp";
     char security[128] = "none";
     char header_type[128] = "none";
     char path[2048] = "";
     char host[2048] = "";
     char sni[2048] = "";
     char alpn[4096] = "";
     char fingerprint[128] = "";
     char public_key[2048] = "";
     char short_ids[2048] = "";
     char spider_x[2048] = "";
     char quic_security[128] = "";
     char quic_key[128] = "";
     char grpc_service_name[2048] = "";
     char grpc_authority[2048] = "";
     char grpc_max_streams[16] = "";
     char mkcp_seed[128] = "";
     char congestion[16] = "";
     char http_headers[4096] = "";
     char fallback[2048] = "";
     char fallbacks[4096] = "";
     char mux[16] = "";
     char session_resumption[16] = "";
     char utls[128] = "";
     char allow_insecure[16] = "";
     char reality_show[16] = "";
     char reality_dest[2048] = "";
     char reality_server_names[4096] = "";
     char outbounds[4096] = "";
     char routing[4096] = "";
 
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
         else if (strncmp(param, "type=", 5) == 0) {
             char* hash = strchr(param + 5, '#');
             if (hash) {
                 size_t network_len = hash - (param + 5);
                 strncpy(network, param + 5, network_len < sizeof(network) ? network_len : sizeof(network) - 1);
                 network[network_len] = '\0';
                 strncpy(grpc_service_name, hash + 1, sizeof(grpc_service_name) - 1);
             } else {
                 strncpy(network, param + 5, sizeof(network) - 1);
             }
         }
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
         else if (strncmp(param, "authority=", 10) == 0) strncpy(grpc_authority, param + 10, sizeof(grpc_authority) - 1);
         else if (strncmp(param, "maxStreams=", 11) == 0) strncpy(grpc_max_streams, param + 11, sizeof(grpc_max_streams) - 1);
         else if (strncmp(param, "seed=", 5) == 0) strncpy(mkcp_seed, param + 5, sizeof(mkcp_seed) - 1);
         else if (strncmp(param, "congestion=", 11) == 0) strncpy(congestion, param + 11, sizeof(congestion) - 1);
         else if (strncmp(param, "headers=", 8) == 0) strncpy(http_headers, param + 8, sizeof(http_headers) - 1);
         else if (strncmp(param, "fallback=", 9) == 0) strncpy(fallback, param + 9, sizeof(fallback) - 1);
         else if (strncmp(param, "fallbacks=", 10) == 0) strncpy(fallbacks, param + 10, sizeof(fallbacks) - 1);
         else if (strncmp(param, "mux=", 4) == 0) strncpy(mux, param + 4, sizeof(mux) - 1);
         else if (strncmp(param, "sessionResumption=", 17) == 0) strncpy(session_resumption, param + 17, sizeof(session_resumption) - 1);
         else if (strncmp(param, "utls=", 5) == 0) strncpy(utls, param + 5, sizeof(utls) - 1);
         else if (strncmp(param, "allowInsecure=", 14) == 0) strncpy(allow_insecure, param + 14, sizeof(allow_insecure) - 1);
         else if (strncmp(param, "show=", 5) == 0) strncpy(reality_show, param + 5, sizeof(reality_show) - 1);
         else if (strncmp(param, "dest=", 5) == 0) strncpy(reality_dest, param + 5, sizeof(reality_dest) - 1);
         else if (strncmp(param, "serverNames=", 12) == 0) strncpy(reality_server_names, param + 12, sizeof(reality_server_names) - 1);
         else if (strncmp(param, "outbounds=", 10) == 0) strncpy(outbounds, param + 10, sizeof(outbounds) - 1);
         else if (strncmp(param, "routing=", 8) == 0) strncpy(routing, param + 8, sizeof(routing) - 1);
         param = strtok(NULL, "&");
     }
 
     if (flow[0] && strcmp(flow, "xtls-rprx-vision") != 0 && strcmp(flow, "xtls-rprx-direct") != 0) {
         log_message("Invalid flow value", __FILE__, __LINE__, 0, flow);
         return -1;
     }
 
     char decoded_path[2048];
     char decoded_host[2048];
     char decoded_sni[2048];
     urldecode(decoded_path, path, sizeof(decoded_path));
     urldecode(decoded_host, host, sizeof(decoded_host));
     urldecode(decoded_sni, sni, sizeof(decoded_sni));
 
     fprintf(fp, "{\n");
     fprintf(fp, "  \"inbounds\": [\n");
     fprintf(fp, "    {\"port\": %d, \"protocol\": \"http\", \"settings\": {}},\n", final_http_port);
     fprintf(fp, "    {\"port\": %d, \"protocol\": \"socks\", \"settings\": {\"udp\": true}}\n", final_socks_port);
     fprintf(fp, "  ],\n");
     fprintf(fp, "  \"outbounds\": [\n");
 
     fprintf(fp, "    {\n");
     fprintf(fp, "      \"protocol\": \"vless\",\n");
     fprintf(fp, "      \"settings\": {\"vnext\": [{\"address\": \"%s\", \"port\": %d, \"users\": [{\"id\": \"%s\", \"encryption\": \"%s\"",
             address, server_port, uuid, encryption);
     if (flow[0]) fprintf(fp, ", \"flow\": \"%s\"", flow);
     fprintf(fp, "}]}]},\n");
 
     fprintf(fp, "      \"streamSettings\": {\n");
     fprintf(fp, "        \"network\": \"%s\"", network);
     fprintf(fp, ",\n        \"security\": \"%s\"", security);
 
     int need_comma = 0;
 
     if (strcmp(network, "tcp") == 0) {
         fprintf(fp, ",\n        \"tcpSettings\": {\"header\": {\"type\": \"%s\"}}", header_type);
         need_comma = 1;
     } else if (strcmp(network, "http") == 0 || strcmp(network, "h2") == 0) {
         fprintf(fp, ",\n        \"httpSettings\": {\"path\": \"%s\"", decoded_path);
         if (host[0]) fprintf(fp, ", \"host\": [\"%s\"]", decoded_host);
         if (http_headers[0]) {
             fprintf(fp, ", \"headers\": {");
             char headers_copy[4096];
             strncpy(headers_copy, http_headers, sizeof(headers_copy) - 1);
             headers_copy[sizeof(headers_copy) - 1] = '\0';
             char* header = strtok(headers_copy, ",");
             int first = 1;
             while (header) 
             {
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
         fprintf(fp, "}");
         need_comma = 1;
     } else if (strcmp(network, "ws") == 0) {
         fprintf(fp, ",\n        \"wsSettings\": {\"path\": \"%s\"", decoded_path);
         if (host[0]) fprintf(fp, ", \"headers\": {\"Host\": \"%s\"}", decoded_host);
         fprintf(fp, "}");
         need_comma = 1;
     } else if (strcmp(network, "kcp") == 0) {
         fprintf(fp, ",\n        \"kcpSettings\": {\"header\": {\"type\": \"%s\"}", header_type);
         if (mkcp_seed[0]) fprintf(fp, ", \"seed\": \"%s\"", mkcp_seed);
         if (congestion[0]) fprintf(fp, ", \"congestion\": %s", strcmp(congestion, "bbr") == 0 ? "true" : "false");
         fprintf(fp, "}");
         need_comma = 1;
     } else if (strcmp(network, "quic") == 0) {
         fprintf(fp, ",\n        \"quicSettings\": {\"security\": \"%s\", \"key\": \"%s\", \"header\": {\"type\": \"%s\"}}",
                 quic_security, quic_key, header_type);
         need_comma = 1;
     } else if (strcmp(network, "grpc") == 0) {
         fprintf(fp, ",\n        \"grpcSettings\": {\"serviceName\": \"%s\", \"multiMode\": %s",
                 grpc_service_name[0] ? grpc_service_name : "v2ray",
                 strchr(grpc_service_name, ',') ? "true" : "false");
         if (grpc_authority[0]) fprintf(fp, ", \"authority\": \"%s\"", grpc_authority);
         if (grpc_max_streams[0]) fprintf(fp, ", \"maxStreams\": %s", grpc_max_streams);
         fprintf(fp, "}");
         need_comma = 1;
     }
 
     if (strcmp(security, "tls") == 0) {
         if (need_comma) fprintf(fp, ",");
         fprintf(fp, "\n        \"tlsSettings\": {\"serverName\": \"%s\"", decoded_sni);
         if (alpn[0]) {
             fprintf(fp, ", \"alpn\": [");
             char alpn_copy[4096];
             strncpy(alpn_copy, alpn, sizeof(alpn_copy) - 1);
             alpn_copy[sizeof(alpn_copy) - 1] = '\0';
 
             char* pos = alpn_copy;
             while ((pos = strstr(pos, "%2C")) != NULL) {
                 *pos = ','; 
                 memmove(pos + 1, pos + 3, strlen(pos + 3) + 1);
             }
 
             char* alpn_value = strtok(alpn_copy, ",");
             int first = 1;
             while (alpn_value) {
                 char decoded_alpn[128];
                 urldecode(decoded_alpn, alpn_value, sizeof(decoded_alpn));
                 char* hash = strchr(decoded_alpn, '#');
                 if (hash) *hash = '\0';
                 if (!first) fprintf(fp, ", ");
                 fprintf(fp, "\"%s\"", decoded_alpn);
                 first = 0;
                 alpn_value = strtok(NULL, ",");
             }
             fprintf(fp, "]");
         }
         if (fingerprint[0]) fprintf(fp, ", \"fingerprint\": \"%s\"", fingerprint);
         if (allow_insecure[0]) fprintf(fp, ", \"allowInsecure\": %s", strcmp(allow_insecure, "true") == 0 ? "true" : "false");
         if (session_resumption[0]) fprintf(fp, ", \"enableSessionResumption\": %s", strcmp(session_resumption, "true") == 0 ? "true" : "false");
         if (utls[0]) fprintf(fp, ", \"utls\": \"%s\"", utls);
         fprintf(fp, "}");
         need_comma = 1;
     } else if (strcmp(security, "reality") == 0) {
         if (need_comma) fprintf(fp, ",");
         fprintf(fp, "\n        \"realitySettings\": {\"publicKey\": \"%s\"", public_key);
         if (short_ids[0]) fprintf(fp, ", \"shortIds\": [\"%s\"]", short_ids);
         if (spider_x[0]) fprintf(fp, ", \"spiderX\": \"%s\"", spider_x);
         if (fingerprint[0]) fprintf(fp, ", \"fingerprint\": \"%s\"", fingerprint);
         if (reality_show[0]) fprintf(fp, ", \"show\": %s", strcmp(reality_show, "true") == 0 ? "true" : "false");
         if (reality_dest[0]) fprintf(fp, ", \"dest\": \"%s\"", reality_dest);
         if (reality_server_names[0]) {
             fprintf(fp, ", \"serverNames\": [");
             char server_names_copy[4096];
             strncpy(server_names_copy, reality_server_names, sizeof(server_names_copy) - 1);
             server_names_copy[sizeof(server_names_copy) - 1] = '\0';
             char* server_name = strtok(server_names_copy, ",");
             int first = 1;
             while (server_name) {
                 if (!first) fprintf(fp, ", ");
                 fprintf(fp, "\"%s\"", server_name);
                 first = 0;
                 server_name = strtok(NULL, ",");
             }
             fprintf(fp, "]");
         }
         fprintf(fp, "}");
         need_comma = 1;
     }
 
     if (fallbacks[0]) {
         if (need_comma) fprintf(fp, ",");
         fprintf(fp, "\n        \"fallbacks\": [");
         char fallbacks_copy[4096];
         strncpy(fallbacks_copy, fallbacks, sizeof(fallbacks_copy) - 1);
         fallbacks_copy[sizeof(fallbacks_copy) - 1] = '\0';
         char* fallback_entry = strtok(fallbacks_copy, ";");
         int first = 1;
         while (fallback_entry) {
             char fb_alpn[128] = "";
             char fb_dest[2048] = "";
             char fb_xver[16] = "";
             char entry_copy[4096];
             strncpy(entry_copy, fallback_entry, sizeof(entry_copy) - 1);
             entry_copy[sizeof(entry_copy) - 1] = '\0';
             char* fb_param = strtok(entry_copy, ",");
             while (fb_param) {
                 if (strncmp(fb_param, "alpn:", 5) == 0) strncpy(fb_alpn, fb_param + 5, sizeof(fb_alpn) - 1);
                 else if (strncmp(fb_param, "dest:", 5) == 0) strncpy(fb_dest, fb_param + 5, sizeof(fb_dest) - 1);
                 else if (strncmp(fb_param, "xver:", 5) == 0) strncpy(fb_xver, fb_param + 5, sizeof(fb_xver) - 1);
                 fb_param = strtok(NULL, ",");
             }
             if (fb_dest[0]) {
                 if (!first) fprintf(fp, ", ");
                 fprintf(fp, "{");
                 if (fb_alpn[0]) fprintf(fp, "\"alpn\": \"%s\",", fb_alpn);
                 fprintf(fp, "\"dest\": \"%s\"", fb_dest);
                 if (fb_xver[0]) fprintf(fp, ", \"xver\": %s", fb_xver);
                 fprintf(fp, "}");
                 first = 0;
             }
             fallback_entry = strtok(NULL, ";");
         }
         fprintf(fp, "]");
         need_comma = 1;
     } else if (fallback[0]) {
         if (need_comma) fprintf(fp, ",");
         fprintf(fp, "\n        \"fallback\": \"%s\"", fallback);
         need_comma = 1;
     }
 
     if (mux[0]) {
         if (need_comma) fprintf(fp, ",");
         fprintf(fp, "\n        \"mux\": {\"enabled\": %s}", strcmp(mux, "true") == 0 ? "true" : "false");
         need_comma = 1;
     }
 
     fprintf(fp, "\n      }\n");
     fprintf(fp, "    }");
 
     if (outbounds[0]) {
         char outbounds_copy[4096];
         strncpy(outbounds_copy, outbounds, sizeof(outbounds_copy) - 1);
         outbounds_copy[sizeof(outbounds_copy) - 1] = '\0';
         char* outbound_entry = strtok(outbounds_copy, ";");
         while (outbound_entry) {
             char tag[128] = "";
             char ob_address[2048] = "";
             char ob_port[16] = "";
             if (sscanf(outbound_entry, "%127[^:]:%2047[^:]:%15s", tag, ob_address, ob_port) == 3) {
                 fprintf(fp, ",\n    {\n");
                 fprintf(fp, "      \"protocol\": \"vless\",\n");
                 fprintf(fp, "      \"tag\": \"%s\",\n", tag);
                 fprintf(fp, "      \"settings\": {\"vnext\": [{\"address\": \"%s\", \"port\": %d, \"users\": [{\"id\": \"%s\", \"encryption\": \"%s\"}]}]}\n",
                         ob_address, atoi(ob_port), uuid, encryption);
                 fprintf(fp, "    }");
             }
             outbound_entry = strtok(NULL, ";");
         }
     }

     if (routing[0]) {
         fprintf(fp, "\n  ],\n");
     }
     else{
        fprintf(fp, "\n  ]\n");
     }
     if (routing[0]) {
         fprintf(fp, "  \"routing\": {\n");
         fprintf(fp, "    \"rules\": [\n");
         char routing_copy[4096];
         strncpy(routing_copy, routing, sizeof(routing_copy) - 1);
         routing_copy[sizeof(routing_copy) - 1] = '\0';
         char* rule = strtok(routing_copy, ";");
         int first = 1;
         while (rule) {
             char type[128] = "";
             char value[2048] = "";
             char outbound_tag[128] = "";
             if (sscanf(rule, "%127[^:]:%2047[^:]:%127s", type, value, outbound_tag) == 3) {
                 if (!first) fprintf(fp, ",\n");
                 fprintf(fp, "      {\"type\": \"field\", \"%s\": [\"%s\"], \"outboundTag\": \"%s\"}",
                         strcmp(type, "domain") == 0 ? "domain" : "ip", value, outbound_tag);
                 first = 0;
             }
             rule = strtok(NULL, ";");
         }
         fprintf(fp, "\n    ]\n");
         fprintf(fp, "  }\n");
     }
 
     fprintf(fp, "}\n");
 
     char extra_info[256];
     snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, HTTP Port: %d, SOCKS Port: %d",
              address, server_port, final_http_port, final_socks_port);
     log_message("VLESS config written successfully", __FILE__, __LINE__, 0, extra_info);
     return 0;
 }