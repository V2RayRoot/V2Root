#ifndef LIBV2ROOT_COMMON_H
#define LIBV2ROOT_COMMON_H

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

#ifdef _WIN32
    typedef unsigned long PID_TYPE;
#else
    typedef int PID_TYPE;
#endif

/*
 * Logs a message with file, line, error code, and optional extra information.
 *
 * Parameters:
 *   message (const char*): The log message.
 *   file (const char*): The source file name.
 *   line (int): The line number.
 *   err_code (int): The error code (e.g., errno or Windows error).
 *   extra_info (const char*): Optional additional information (can be NULL).
 */
EXPORT void log_message(const char* message, const char* file, int line, int err_code, const char* extra_info);

#endif