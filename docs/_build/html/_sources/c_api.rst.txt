C API Usage
=================================

This section explains how to use the V2ROOT C API in other projects by leveraging the shared libraries (``libv2root.dll`` on Windows, ``libv2root.so`` on Linux). These libraries allow developers to integrate V2ROOT's functionality into applications written in languages like C, C++, or any language that supports loading shared libraries (e.g., Python via ``ctypes``, Rust via FFI).

Overview of the C API
---------------------

The V2ROOT C API provides a set of functions to manage V2Ray proxy operations, including initialization, starting/stopping the proxy, testing configurations, and pinging servers.

The shared libraries are built from the C codebase and expose the following functions. Each function returns an integer: 0 for success or a negative error code for failure (refer to the :ref:`api` section for error code details).

API Functions
-------------

Below is a detailed list of the available functions, their signatures, inputs, and outputs:

- **init_v2ray(config_file: char*, v2ray_path: char*) -> int**:

  Initializes the V2Ray core with a configuration file and the path to the V2Ray executable.

  - **Inputs**:

    - ``config_file``: A null-terminated string specifying the path to the V2Ray configuration file (e.g., ``"config.json"``).

    - ``v2ray_path``: A null-terminated string specifying the path to the V2Ray executable (e.g., ``"v2ray"`` on Linux or ``"v2ray.exe"`` on Windows).

  - **Output**:

    - Returns 0 on success, or a negative error code on failure (e.g., -1 for general failure, -5 for initialization error).

- **reset_network_proxy() -> int**:

  Resets the system network proxy settings.

  - **Inputs**:

    None.

  - **Output**:

    - Returns 0 on success, or a negative error code on failure (e.g., -6 for proxy setting error).

- **parse_config_string(config_str: char*, http_port: int, socks_port: int) -> int**:

  Parses a V2Ray configuration string (e.g., VLESS or VMess) and sets up the proxy with the specified ports.

  - **Inputs**:

    - ``config_str``: A null-terminated string containing the V2Ray configuration (e.g., ``"vless://user-id@server:443?security=tls&type=tcp#MyVLESS"``).

    - ``http_port``: The HTTP proxy port (e.g., 2300).

    - ``socks_port``: The SOCKS proxy port (e.g., 2301).

  - **Output**:

    - Returns 0 on success, or a negative error code on failure (e.g., -3 for invalid configuration).

- **start_v2ray(http_port: int, socks_port: int) -> int**:

  Starts the V2Ray proxy service on the specified ports.

  - **Inputs**:

    - ``http_port``: The HTTP proxy port (e.g., 2300).

    - ``socks_port``: The SOCKS proxy port (e.g., 2301).

  - **Output**:

    - Returns the process ID (PID) of the V2Ray process on success, or a negative error code on failure (e.g., -2 for service operation failure).

- **stop_v2ray() -> int**:

  Stops the V2Ray proxy service.

  - **Inputs**:

    None.

  - **Output**:

    - Returns 0 on success, or a negative error code on failure (e.g., -2 for service operation failure).

- **test_config_connection(config_str: char*, latency: int*, http_port: int, socks_port: int) -> int**:

  Tests the connectivity and latency of a V2Ray configuration.

  - **Inputs**:

    - ``config_str``: A null-terminated string containing the V2Ray configuration (e.g., ``"vless://user-id@server:443?security=tls&type=tcp#MyVLESS"``).

    - ``latency``: A pointer to an integer where the latency (in milliseconds) will be stored.

    - ``http_port``: The HTTP proxy port (e.g., 2300).

    - ``socks_port``: The SOCKS proxy port (e.g., 2301).

  - **Output**:

    - Returns 0 on success, or a negative error code on failure (e.g., -4 for network error). The latency value is stored in the ``latency`` pointer on success.

- **ping_server(address: char*, port: int) -> int**:

  Pings a server to measure latency.

  - **Inputs**:

    - ``address``: A null-terminated string specifying the server address (e.g., ``"example.com"``).

    - ``port``: The port to connect to (e.g., 443).

  - **Output**:

    - Returns the latency in milliseconds on success, or a negative error code on failure (e.g., -4 for network error).

Example: Using the C API in a C Program
---------------------------------------

Below is an example of how to use the V2ROOT C API in a C program on Linux. The process is similar on Windows, with adjustments for loading the DLL (using ``LoadLibrary`` and ``GetProcAddress`` instead of ``dlopen`` and ``dlsym``).

.. code-block:: c

   #include <stdio.h>
   #include <dlfcn.h> // For Linux (use windows.h on Windows)

   // Function pointer types for the API functions
   typedef int (*init_v2ray_t)(const char*, const char*);
   typedef int (*parse_config_string_t)(const char*, int, int);
   typedef int (*start_v2ray_t)(int, int);
   typedef int (*test_config_connection_t)(const char*, int*, int, int);
   typedef int (*stop_v2ray_t)(void);

   int main() {

       // Load the shared library
       void* lib = dlopen("/path/to/libv2root.so", RTLD_LAZY); // On Windows: LoadLibrary("libv2root.dll")
       if (!lib) {
           printf("Failed to load library: %s\n", dlerror());
           return 1;
       }

       // Load the functions
       init_v2ray_t init_v2ray = (init_v2ray_t)dlsym(lib, "init_v2ray"); // On Windows: GetProcAddress
       parse_config_string_t parse_config_string = (parse_config_string_t)dlsym(lib, "parse_config_string");
       start_v2ray_t start_v2ray = (start_v2ray_t)dlsym(lib, "start_v2ray");
       test_config_connection_t test_config_connection = (test_config_connection_t)dlsym(lib, "test_config_connection");
       stop_v2ray_t stop_v2ray = (stop_v2ray_t)dlsym(lib, "stop_v2ray");

       if (!init_v2ray || !parse_config_string || !start_v2ray || !test_config_connection || !stop_v2ray) {
           printf("Failed to load functions: %s\n", dlerror());
           dlclose(lib);
           return 1;
       }

       // Initialize V2Ray
       int result = init_v2ray("config.json", "/path/to/v2ray");
       if (result != 0) {
           printf("Failed to initialize V2Ray: %d\n", result);
           dlclose(lib);
           return 1;
       }
       printf("V2Ray initialized successfully!\n");

       // Parse a configuration string
       const char* config = "vless://user-id@server:443?security=tls&type=tcp#MyVLESS";
       result = parse_config_string(config, 2300, 2301);
       if (result != 0) {
           printf("Failed to parse config: %d\n", result);
           dlclose(lib);
           return 1;
       }
       printf("Configuration parsed successfully!\n");

       // Start the V2Ray proxy
       int pid = start_v2ray(2300, 2301);
       if (pid < 0) {
           printf("Failed to start V2Ray: %d\n", pid);
           dlclose(lib);
           return 1;
       }
       printf("V2Ray started with PID: %d\n", pid);

       // Test the configuration
       int latency = 0;
       result = test_config_connection(config, &latency, 2300, 2301);
       if (result != 0) {
           printf("Connection test failed: %d\n", result);
       } else {
           printf("Connection test successful, latency: %d ms\n", latency);
       }

       // Stop the V2Ray proxy
       result = stop_v2ray();
       if (result != 0) {
           printf("Failed to stop V2Ray: %d\n", result);
       } else {
           printf("V2Ray stopped successfully!\n");
       }

       // Clean up
       dlclose(lib);
       return 0;
   }

Notes
-----

- **Windows Adaptation**:

  On Windows, replace ``dlopen`` with ``LoadLibrary`` and ``dlsym`` with ``GetProcAddress``. For example:

  .. code-block:: c

     #include <windows.h>

     HINSTANCE lib = LoadLibrary("libv2root.dll");
     init_v2ray_t init_v2ray = (init_v2ray_t)GetProcAddress(lib, "init_v2ray");

- **Error Handling**:

  Always check the return values of the functions. Negative values indicate errors, which can be interpreted using the error codes documented in the :ref:`api` section.

- **Paths**:

  Ensure the paths to ``libv2root.so`` (or ``libv2root.dll``) and the V2Ray executable are correct for your system.

- **Dependencies**:

  The shared library depends on V2Ray and system libraries (e.g., ``libjansson.so`` on Linux). Ensure these dependencies are available.

Using the API in Other Languages
--------------------------------

The C API can be used in other languages that support foreign function interfaces (FFI):

- **Python**:

  Use the ``ctypes`` module to load the shared library and call the functions, similar to how the ``v2root.py`` file interacts with the library.

- **Rust**:

  Use the ``libloading`` crate or FFI bindings to load and call the functions.

- **Go**:

  Use the ``cgo`` package to interface with the C API.

For detailed usage in these languages, refer to their respective FFI documentation, ensuring you handle the same inputs and outputs as described above.