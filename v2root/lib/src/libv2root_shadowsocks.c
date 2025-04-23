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
#include "libv2root_shadowsocks.h"
#include "libv2root_core.h"
#include "libv2root_utils.h"

/*
 * Retrieves a query parameter value from a query string.
 *
 * Searches for the specified key in the query string and extracts its value.
 *
 * Parameters:
 *   query (const char*): The query string to search.
 *   key (const char*): The key to find.
 *   value (char*): Buffer to store the extracted value.
 *   value_size (size_t): Size of the value buffer.
 *
 * Returns:
 *   char*: Pointer to the value buffer if found, NULL otherwise.
 *
 * Errors:
 *   None
 */

static char* get_query_param(const char* query, const char* key, char* value, size_t value_size) {
    const char* param = strstr(query, key);
    if (!param) return NULL;

    param += strlen(key);
    if (*param != '=') return NULL;
    param++;

    const char* end = strchr(param, '&');
    if (!end) end = param + strlen(param);

    size_t len = end - param;
    if (len >= value_size) len = value_size - 1;

    strncpy(value, param, len);
    value[len] = '\0';
    return value;
}

/*
 * Retrieves a query parameter value from a query string.
 *
 * Searches for the specified key in the query string and extracts its value.
 *
 * Parameters:
 *   query (const char*): The query string to search.
 *   key (const char*): The key to find.
 *   value (char*): Buffer to store the extracted value.
 *   value_size (size_t): Size of the value buffer.
 *
 * Returns:
 *   char*: Pointer to the value buffer if found, NULL otherwise.
 *
 * Errors:
 *   None
 */

EXPORT int parse_shadowsocks_string(const char* ss_str, FILE* fp, int http_port, int socks_port) {
    if (ss_str == NULL || fp == NULL) {
        log_message("Null ss_str or fp", __FILE__, __LINE__, 0, NULL);
        return -1;
    }
    if (strncmp(ss_str, "ss://", 5) != 0) {
        log_message("Invalid Shadowsocks prefix", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    char http_port_str[16];
    char socks_port_str[16];
    snprintf(http_port_str, sizeof(http_port_str), "%d", http_port);
    snprintf(socks_port_str, sizeof(socks_port_str), "%d", socks_port);

    int final_http_port = (http_port > 0 && validate_port(http_port_str)) ? http_port : DEFAULT_HTTP_PORT;
    int final_socks_port = (socks_port > 0 && validate_port(socks_port_str)) ? socks_port : DEFAULT_SOCKS_PORT;

    const char* base64_data = ss_str + 5;
    char* at_sign = strchr(base64_data, '@');
    if (!at_sign) {
        log_message("Invalid Shadowsocks format: no @ found", __FILE__, __LINE__, 0, NULL);
        return -1;
    }

    size_t base64_len = at_sign - base64_data;
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

    char method[128] = "";
    char password[128] = "";
    if (sscanf(decoded, "%127[^:]:%127s", method, password) != 2) {
        log_message("Invalid method:password format", __FILE__, __LINE__, 0, decoded);
        free(decoded);
        return -1;
    }
    free(decoded);

    char address[2048] = "";
    char port_str[16] = "";
    const char* query_start = strchr(at_sign, '#');
    if (!query_start) query_start = at_sign + strlen(at_sign);

    char* colon = strchr(at_sign + 1, ':');
    if (!colon || colon >= query_start) {
        log_message("Invalid address:port format", __FILE__, __LINE__, 0, at_sign);
        return -1;
    }

    size_t addr_len = colon - (at_sign + 1);
    if (addr_len >= sizeof(address)) addr_len = sizeof(address) - 1;
    strncpy(address, at_sign + 1, addr_len);
    address[addr_len] = '\0';

    const char* port_start = colon + 1;
    size_t port_len = query_start - port_start;
    if (port_len >= sizeof(port_str)) port_len = sizeof(port_str) - 1;
    strncpy(port_str, port_start, port_len);
    port_str[port_len] = '\0';

    if (!validate_address(address)) {
        log_message("Invalid address format", __FILE__, __LINE__, 0, address);
        return -1;
    }
    if (!validate_port(port_str)) {
        log_message("Invalid port", __FILE__, __LINE__, 0, port_str);
        return -1;
    }
    int server_port = atoi(port_str);

    char plugin[128] = "";
    char plugin_opts[1024] = "";
    char tag[128] = "";
    char level[16] = "0";
    char ota[16] = "false";
    char network[16] = "tcp";
    char security[16] = "";

    const char* query = strchr(ss_str, '?');
    if (query && query < query_start) {
        query++;
        if (get_query_param(query, "plugin", plugin, sizeof(plugin))) {
            get_query_param(query, "plugin-opts", plugin_opts, sizeof(plugin_opts));
        }
        get_query_param(query, "tag", tag, sizeof(tag));
        get_query_param(query, "level", level, sizeof(level));
        get_query_param(query, "ota", ota, sizeof(ota));
        get_query_param(query, "network", network, sizeof(network));
        get_query_param(query, "security", security, sizeof(security));
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"inbounds\": [\n");
    fprintf(fp, "    {\"port\": %d, \"protocol\": \"http\", \"settings\": {}},\n", final_http_port);
    fprintf(fp, "    {\"port\": %d, \"protocol\": \"socks\", \"settings\": {\"udp\": true}}\n", final_socks_port);
    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"outbounds\": [{\n");
    fprintf(fp, "    \"protocol\": \"shadowsocks\",\n");
    fprintf(fp, "    \"settings\": {\n");
    fprintf(fp, "      \"servers\": [{\n");
    fprintf(fp, "        \"address\": \"%s\",\n", address);
    fprintf(fp, "        \"port\": %d,\n", server_port);
    fprintf(fp, "        \"method\": \"%s\",\n", method);
    fprintf(fp, "        \"password\": \"%s\",\n", password);
    fprintf(fp, "        \"ota\": %s,\n", strcmp(ota, "true") == 0 ? "true" : "false");
    fprintf(fp, "        \"level\": %d\n", atoi(level));
    fprintf(fp, "      }]\n");
    fprintf(fp, "    },\n");

    if (plugin[0]) {
        fprintf(fp, "    \"streamSettings\": {\n");
        fprintf(fp, "      \"network\": \"%s\",\n", network);
        if (security[0]) {
            fprintf(fp, "      \"security\": \"%s\",\n", security);
        }
        fprintf(fp, "      \"plugin\": \"%s\",\n", plugin);
        if (plugin_opts[0]) {
            fprintf(fp, "      \"pluginOpts\": \"%s\"\n", plugin_opts);
        }
        fprintf(fp, "    },\n");
    } else {
        fprintf(fp, "    \"streamSettings\": {\n");
        fprintf(fp, "      \"network\": \"%s\"\n", network);
        if (security[0]) {
            fprintf(fp, "      ,\"security\": \"%s\"\n", security);
        }
        fprintf(fp, "    },\n");
    }

    if (tag[0]) {
        fprintf(fp, "    \"tag\": \"%s\",\n", tag);
    }

    fprintf(fp, "    \"protocol\": \"shadowsocks\"\n");
    fprintf(fp, "  }]\n");
    fprintf(fp, "}\n");

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, Method: %s, HTTP Port: %d, SOCKS Port: %d",
            address, server_port, method, final_http_port, final_socks_port);
    log_message("Shadowsocks config with full options written successfully", __FILE__, __LINE__, 0, extra_info);
    return 0;
}