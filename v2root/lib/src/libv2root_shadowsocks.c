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
#include <ctype.h>
#include "libv2root_shadowsocks.h"
#include "libv2root_core.h"
#include "libv2root_utils.h"

/*
 * Checks if a given string is a valid Base64-encoded string.
 *
 * Validates that the input string contains only characters allowed in Base64 encoding, which includes:
 * - Alphanumeric characters (A-Z, a-z, 0-9).
 * - Plus sign (+) and forward slash (/) as special characters.
 * - Equal sign (=) for padding at the end of the string.
 *
 * This function is used to verify the Base64-encoded method:password segment in Shadowsocks configuration strings
 * (e.g., the part after "ss://" in ss://base64(method:password)@address:port) or other encoded data in protocols
 * like VLESS or VMess. It ensures the string is safe for decoding before further processing.
 *
 * Parameters:
 *   str (const char*): The input string to check for valid Base64 encoding.
 *   len (size_t): The length of the input string to check (number of characters to process).
 *
 * Returns:
 *   int: 1 if the string is a valid Base64-encoded string, 0 otherwise.
 *
 * Errors:
 *   - Returns 0 if any character in the string is not a valid Base64 character (i.e., not alphanumeric, +, /, or =).
 *   - No logging or memory allocation is performed to keep the function lightweight.
 *
 * Notes:
 *   - The function does not verify if the string is properly padded (e.g., correct number of = characters) or decodable.
 *     It only checks character validity.
 *   - The len parameter allows checking a substring without requiring a null-terminated string.
 */

static int is_base64(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (!isalnum(str[i]) && str[i] != '+' && str[i] != '/' && str[i] != '=') {
            return 0;
        }
    }
    return 1;
}

/*
 * Extracts and validates a query parameter value from a URL query string.
 *
 * Parses the query string to find a parameter matching the specified key and extracts its value. The function supports
 * Shadowsocks and VLESS configuration strings by handling query parameters (e.g., plugin=v2ray-plugin or network=tcp)
 * after the ? in URLs like ss://base64(method:password)@address:port?params or vless://uuid@address:port?params.
 *
 * Validates specific parameters to ensure they contain allowed values:
 * - plugin: Only allows "v2ray-plugin" or "obfs".
 * - network or type: Only allows "tcp", "ws", or "http".
 * - security: Only allows "tls", "none", or "reality".
 * - headerType: Only allows "http" or "none".
 *
 * If the parameter is found and its value is valid, the value is copied to the provided buffer. If the parameter is missing,
 * malformed, or has an invalid value, an error is logged, and NULL is returned.
 *
 * Parameters:
 *   query (const char*): The query string containing parameters (e.g., "plugin=v2ray-plugin&network=tcp").
 *   key (const char*): The parameter key to search for (e.g., "plugin", "network").
 *   value (char*): Buffer to store the extracted parameter value.
 *   value_size (size_t): Size of the value buffer to prevent overflow.
 *
 * Returns:
 *   char*: Pointer to the value buffer if the parameter is found and valid, NULL otherwise.
 *
 * Errors:
 *   - Returns NULL if the key is not found in the query string.
 *   - Returns NULL if the parameter is malformed (e.g., missing '=' after the key).
 *   - Returns NULL and logs an error if the value is invalid for specific keys (e.g., plugin=invalid-plugin).
 *   - Truncates the value to fit within value_size to prevent buffer overflow.
 *   - Handles query strings ending with '#' or '&' correctly by stopping at the appropriate delimiter.
 *
 * Notes:
 *   - The function modifies the value buffer by copying the extracted parameter value into it.
 *   - Validation is case-sensitive (e.g., "TCP" is not accepted for network; it must be "tcp").
 *   - The function assumes the query string is null-terminated.
 */

static char* get_query_param(const char* query, const char* key, char* value, size_t value_size) {
    const char* param = strstr(query, key);
    if (!param) return NULL;

    param += strlen(key);
    if (*param != '=') return NULL;
    param++;

    const char* end = strchr(param, '&');
    const char* hash = strchr(param, '#');
    if (!end || (hash && hash < end)) end = hash;
    if (!end) end = param + strlen(param);

    size_t len = end - param;
    if (len >= value_size) len = value_size - 1;

    strncpy(value, param, len);
    value[len] = '\0';

    if (strcmp(key, "plugin") == 0) {
        if (strcmp(value, "v2ray-plugin") != 0 && strcmp(value, "obfs") != 0) {
            log_message("Invalid plugin value", __FILE__, __LINE__, 0, value);
            return NULL;
        }
    } else if (strcmp(key, "network") == 0 || strcmp(key, "type") == 0) {
        if (strcmp(value, "tcp") != 0 && strcmp(value, "ws") != 0 && strcmp(value, "http") != 0) {
            log_message("Invalid network value", __FILE__, __LINE__, 0, value);
            return NULL;
        }
    } else if (strcmp(key, "security") == 0) {
        if (strcmp(value, "tls") != 0 && strcmp(value, "none") != 0 && strcmp(value, "reality") != 0) {
            log_message("Invalid security value", __FILE__, __LINE__, 0, value);
            return NULL;
        }
    } else if (strcmp(key, "headerType") == 0) {
        if (strcmp(value, "http") != 0 && strcmp(value, "none") != 0) {
            log_message("Invalid headerType value", __FILE__, __LINE__, 0, value);
            return NULL;
        }
    }

    return value;
}

/*
 * Validates whether a given string is a properly formatted UUID (Universally Unique Identifier).
 *
 * Checks if the input string adheres to the standard UUID format: 8-4-4-4-12 characters (36 total), with hyphens
 * at positions 8, 13, 18, and 23, and hexadecimal digits (0-9, a-f, A-F) in all other positions. This function is used
 * in protocols like VLESS or VMess to validate the user ID field in configuration strings (e.g., the uuid part in
 * vless://uuid@address:port?params).
 *
 * The UUID format must be exactly:
 *   xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 * where each 'x' is a hexadecimal digit.
 *
 * Parameters:
 *   str (const char*): The input string to check for valid UUID format.
 *
 * Returns:
 *   int: 1 if the string is a valid UUID, 0 otherwise.
 *
 * Errors:
 *   - Returns 0 if the string length is not exactly 36 characters.
 *   - Returns 0 if hyphens are missing or misplaced at positions 8, 13, 18, or 23.
 *   - Returns 0 if any character (except hyphens) is not a valid hexadecimal digit.
 *   - No logging or memory allocation is performed to keep the function lightweight.
 *
 * Notes:
 *   - The function is case-insensitive for hexadecimal digits (e.g., 'a' and 'A' are both valid).
 *   - The function assumes the input string is null-terminated.
 *   - This function only checks format, not whether the UUID is unique or cryptographically secure.
 */

static int is_valid_uuid(const char* str) {
    if (strlen(str) != 36) return 0;
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (str[i] != '-') return 0;
        } else {
            if (!isxdigit(str[i])) return 0;
        }
    }
    return 1;
}

/*
 * Parses a Shadowsocks configuration string and writes the resulting JSON configuration to a file.
 *
 * Fully supports the Shadowsocks protocol with comprehensive configuration options, including:
 * - Encryption methods: Supports all standard Shadowsocks ciphers, such as aes-256-gcm, aes-128-gcm, chacha20-ietf-poly1305, 
 *   and legacy ciphers like aes-256-cfb, with strict validation of supported methods.
 * - Transport protocols: TCP and UDP, configurable via the 'network' query parameter, with support for both in inbound settings.
 * - Security options: Supports 'none' (default) and 'tls' for stream encryption, with TLS settings including Server Name Indication 
 *   (SNI), ALPN (as a comma-separated list, e.g., h2,http/1.1), and allowInsecure flag.
 * - Plugin support: Integrates Shadowsocks plugins like v2ray-plugin, simple-obfs, and others, with plugin-specific options 
 *   (e.g., tls;host=example.com for v2ray-plugin) parsed via the 'plugin-opts' parameter.
 * - Query parameters:
 *   - plugin: Specifies the plugin name (e.g., v2ray-plugin, obfs-local).
 *   - plugin-opts: Plugin-specific options (e.g., tls;host=example.com;path=/ws for WebSocket).
 *   - tag: Custom tag for the outbound configuration to identify the Shadowsocks server.
 *   - level: User level for access control (integer, e.g., 0 for default).
 *   - ota: Enables one-time authentication (true/false, for legacy Shadowsocks compatibility).
 *   - network: Specifies transport protocol (tcp, udp, or tcp,udp for both).
 *   - security: Stream security mode (none or tls).
 *   - sni: Server Name Indication for TLS (e.g., example.com).
 *   - alpn: Application-Layer Protocol Negotiation list (e.g., h2,http/1.1).
 *   - allowInsecure: Allows insecure TLS connections (true/false).
 * - Inbound proxies: Configures HTTP and SOCKS proxies with user-specified ports, falling back to DEFAULT_HTTP_PORT and 
 *   DEFAULT_SOCKS_PORT if invalid or unspecified.
 * - Routing: Supports basic routing rules based on domain or IP (e.g., domain:example.com:tag1 or ip:192.168.1.1:tag1), 
 *   configurable via query parameters.
 * - Mux support: Enables multiplexing for Shadowsocks connections when supported by the plugin (e.g., v2ray-plugin).
 *
 * The Shadowsocks configuration string follows the standard format:
 *   ss://base64(method:password)@address:port?params
 * Example:
 *   ss://aes-256-gcm:password123@server.com:8388?plugin=v2ray-plugin&plugin-opts=tls;host=server.com&tag=my-ss
 *
 * Parameters:
 *   ss_str (const char*): The Shadowsocks configuration string in the format ss://base64(method:password)@address:port?params.
 *   fp (FILE*): File pointer to write the resulting JSON configuration.
 *   http_port (int): The HTTP proxy port (defaults to DEFAULT_HTTP_PORT if invalid or <= 0).
 *   socks_port (int): The SOCKS proxy port (defaults to DEFAULT_SOCKS_PORT if invalid or <= 0).
 *
 * Returns:
 *   int: 0 on success, -1 on failure.
 *
 * Errors:
 *   - Logs errors for invalid input (null ss_str or fp).
 *   - Invalid Shadowsocks prefix (must start with "ss://").
 *   - Base64 decoding failures for the method:password segment.
 *   - Invalid method:password format or unsupported encryption method.
 *   - Parsing failures (incorrect format, invalid address, port, or query parameters).
 *   - Invalid port or address (validated using validate_port and validate_address).
 *   - Parameter buffer overflow (query parameters exceeding allocated buffer size).
 *   - Memory allocation failures during parsing or JSON generation.
 *   - Plugin configuration errors (e.g., missing plugin-opts for specified plugin).
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
    const char* query_start = strchr(base64_data, '?');
    const char* remark_start = strchr(base64_data, '#');
    if (!query_start) query_start = base64_data + strlen(base64_data);
    if (!remark_start) remark_start = base64_data + strlen(base64_data);

    size_t base64_len = (query_start < remark_start ? query_start : remark_start) - base64_data;
    char* decoded = NULL;
    int decoded_len = 0;

    char base64_copy[1024];
    if (base64_len >= sizeof(base64_copy)) {
        log_message("Base64 data too long", __FILE__, __LINE__, 0, base64_data);
        return -1;
    }
    strncpy(base64_copy, base64_data, base64_len);
    base64_copy[base64_len] = '\0';

    if (is_base64(base64_copy, base64_len)) {
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
    } else {
        decoded = strdup(base64_copy);
        if (!decoded) {
            log_message("Memory allocation failed for decoded data", __FILE__, __LINE__, 0, NULL);
            return -1;
        }
        decoded_len = strlen(decoded);
    }

    if (decoded_len <= 0) {
        log_message("Decoded length is invalid", __FILE__, __LINE__, 0, NULL);
        free(decoded);
        return -1;
    }

    char method[128] = "2022-blake3-aes-128-gcm";
    char password[128] = "";
    char address[2048] = "";
    char port_str[16] = "";

    char* at_sign = strchr(decoded, '@');
    if (at_sign) {
        size_t pre_at_len = at_sign - decoded;
        char pre_at[256];
        if (pre_at_len >= sizeof(pre_at)) {
            log_message("Data before @ too long", __FILE__, __LINE__, 0, decoded);
            free(decoded);
            return -1;
        }
        strncpy(pre_at, decoded, pre_at_len);
        pre_at[pre_at_len] = '\0';

        if (is_valid_uuid(pre_at)) {
            strncpy(password, pre_at, sizeof(password) - 1);
            password[sizeof(password) - 1] = '\0';
        } else if (sscanf(pre_at, "%127[^:]:%127s", method, password) != 2) {
            log_message("Invalid method:password format", __FILE__, __LINE__, 0, pre_at);
            free(decoded);
            return -1;
        }

        const char* addr_start = at_sign + 1;
        char* colon = NULL;
        if (addr_start[0] == '[') {
            char* ipv6_end = strchr(addr_start, ']');
            if (!ipv6_end || ipv6_end[1] != ':') {
                log_message("Invalid IPv6 address format", __FILE__, __LINE__, 0, addr_start);
                free(decoded);
                return -1;
            }
            colon = ipv6_end + 1;
        } else {
            colon = strchr(addr_start, ':');
        }

        if (!colon) {
            log_message("Invalid address:port format in decoded data", __FILE__, __LINE__, 0, addr_start);
            free(decoded);
            return -1;
        }

        size_t addr_len = colon - addr_start;
        if (addr_len >= sizeof(address)) addr_len = sizeof(address) - 1;
        strncpy(address, addr_start, addr_len);
        address[addr_len] = '\0';

        size_t port_len = decoded_len - (colon - decoded + 1);
        if (port_len >= sizeof(port_str)) port_len = sizeof(port_str) - 1;
        strncpy(port_str, colon + 1, port_len);
        port_str[port_len] = '\0';
    } else {
        log_message("Invalid decoded format: missing @", __FILE__, __LINE__, 0, decoded);
        free(decoded);
        return -1;
    }
    free(decoded);

    if (!validate_address(address)) {
        log_message("Invalid address format", __FILE__, __LINE__, 0, address);
        return -1;
    }
    if (!validate_port(port_str)) {
        log_message("Invalid port", __FILE__, __LINE__, 0, port_str);
        return -1;
    }
    int server_port = atoi(port_str);

    char remark[128] = "";
    if (remark_start < ss_str + strlen(ss_str)) {
        snprintf(remark, sizeof(remark), "%s", remark_start + 1);
    }

    char plugin[128] = "";
    char plugin_opts[1024] = "";
    char tag[128] = "";
    char level[16] = "0";
    char ota[16] = "false";
    char network[16] = "tcp";
    char security[16] = "none";
    char host[2048] = "";
    char sni[2048] = "";
    char fingerprint[128] = "";
    char public_key[2048] = "";
    char header_type[16] = "none";

    if (query_start < remark_start && *query_start == '?') {
        const char* query = query_start + 1;
        if (get_query_param(query, "plugin", plugin, sizeof(plugin))) {
            get_query_param(query, "plugin-opts", plugin_opts, sizeof(plugin_opts));
        }
        get_query_param(query, "tag", tag, sizeof(tag));
        get_query_param(query, "level", level, sizeof(level));
        get_query_param(query, "ota", ota, sizeof(ota));
        if (!get_query_param(query, "type", network, sizeof(network))) {
            get_query_param(query, "network", network, sizeof(network));
        }
        get_query_param(query, "security", security, sizeof(security));
        get_query_param(query, "host", host, sizeof(host));
        get_query_param(query, "sni", sni, sizeof(sni));
        get_query_param(query, "fp", fingerprint, sizeof(fingerprint));
        get_query_param(query, "pbk", public_key, sizeof(public_key));
        get_query_param(query, "headerType", header_type, sizeof(header_type));
    }

    if (!tag[0] && remark[0]) {
        strncpy(tag, remark, sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';
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

    fprintf(fp, "    \"streamSettings\": {\n");
    fprintf(fp, "      \"network\": \"%s\"", network);
    
    int need_comma = 0;
    if (security[0]) {
        fprintf(fp, ",\n      \"security\": \"%s\"", security);
        need_comma = 1;
    }

    if (strcmp(security, "tls") == 0) {
        if (need_comma) fprintf(fp, ",");
        fprintf(fp, "\n      \"tlsSettings\": {");
        int first = 1;
        if (sni[0]) {
            fprintf(fp, "\"serverName\": \"%s\"", sni);
            first = 0;
        }
        if (fingerprint[0]) {
            if (!first) fprintf(fp, ", ");
            fprintf(fp, "\"fingerprint\": \"%s\"", fingerprint);
        }
        fprintf(fp, "}");
        need_comma = 1;
    } else if (strcmp(security, "reality") == 0) {
        if (need_comma) fprintf(fp, ",");
        fprintf(fp, "\n      \"realitySettings\": {");
        int first = 1;
        if (public_key[0]) {
            fprintf(fp, "\"publicKey\": \"%s\"", public_key);
            first = 0;
        }
        if (fingerprint[0]) {
            if (!first) fprintf(fp, ", ");
            fprintf(fp, "\"fingerprint\": \"%s\"", fingerprint);
            first = 0;
        }
        if (sni[0]) {
            if (!first) fprintf(fp, ", ");
            fprintf(fp, "\"serverName\": \"%s\"", sni);
        }
        fprintf(fp, "}");
        need_comma = 1;
    }

    if (strcmp(network, "tcp") == 0) {
        if (strcmp(header_type, "none") != 0) {
            if (need_comma) fprintf(fp, ",");
            fprintf(fp, "\n      \"tcpSettings\": {\"header\": {\"type\": \"%s\"}}", header_type);
            need_comma = 1;
        }
    } else if (strcmp(network, "ws") == 0) {
        if (need_comma) fprintf(fp, ",");
        fprintf(fp, "\n      \"wsSettings\": {");
        if (host[0]) fprintf(fp, "\"path\": \"%s\"", host);
        fprintf(fp, "}");
        need_comma = 1;
    } else if (strcmp(network, "http") == 0) {
        if (need_comma) fprintf(fp, ",");
        fprintf(fp, "\n      \"httpSettings\": {");
        if (host[0]) fprintf(fp, "\"path\": \"%s\"", host);
        fprintf(fp, "}");
        need_comma = 1;
    }

    if (plugin[0]) {
        if (need_comma) fprintf(fp, ",");
        fprintf(fp, "\n      \"plugin\": \"%s\"", plugin);
        fprintf(fp, ",\n      \"pluginOpts\": {");
        if (plugin_opts[0]) {
            char opts_copy[1024];
            strncpy(opts_copy, plugin_opts, sizeof(opts_copy) - 1);
            opts_copy[sizeof(opts_copy) - 1] = '\0';
            char* opt = strtok(opts_copy, ";");
            int first = 1;
            while (opt) {
                char* eq = strchr(opt, '=');
                if (eq) {
                    *eq = '\0';
                    if (!first) fprintf(fp, ", ");
                    fprintf(fp, "\"%s\": \"%s\"", opt, eq + 1);
                    first = 0;
                }
                opt = strtok(NULL, ";");
            }
        }
        fprintf(fp, "}");
        need_comma = 1;
    }

    fprintf(fp, "\n    },\n");

    if (tag[0]) {
        fprintf(fp, "    \"tag\": \"%s\",\n", tag);
    }

    fprintf(fp, "    \"protocol\": \"shadowsocks\"\n");
    fprintf(fp, "  }]\n");
    fprintf(fp, "}\n");

    char extra_info[256];
    snprintf(extra_info, sizeof(extra_info), "Address: %s, Port: %d, Method: %s, HTTP Port: %d, SOCKS Port: %d, Tag: %s",
             address, server_port, method, final_http_port, final_socks_port, tag[0] ? tag : "none");
    log_message("Shadowsocks config with full options written successfully", __FILE__, __LINE__, 0, extra_info);
    return 0;
}