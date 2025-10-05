#include <stdio.h>
#include <string.h>
#include "libv2root_core.h"
#include "libv2root_common.h"

/* Core functions are implemented in libv2root_manage.c */
/* This file serves as the main entry point and can contain */
/* additional wrapper functions or shared state management */

/* Global configuration state */
static int g_initialized = 0;

/* 
 * Core initialization wrapper
 * Validates and initializes the V2Ray environment
 */
EXPORT int v2root_init(const char* config_file, const char* v2ray_path) {
    if (g_initialized) {
        return V2ROOT_SUCCESS;
    }
    
    int result = init_v2ray(config_file, v2ray_path);
    if (result == V2ROOT_SUCCESS) {
        g_initialized = 1;
    }
    
    return result;
}

/*
 * Check if V2ROOT is initialized
 */
int is_v2root_initialized(void) {
    return g_initialized;
}

/*
 * Cleanup and reset initialization state
 */
EXPORT void v2root_cleanup(void) {
    stop_v2ray();
    g_initialized = 0;
}
