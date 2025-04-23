import ctypes
import os
import platform
import subprocess
from colorama import init, Fore, Style
import sys
import select

init(autoreset=True)

class V2ROOT:
    """
    A class to manage V2Ray proxy operations on Windows and Linux platforms.

    This class provides an interface to initialize, start, stop, and test V2Ray proxy
    configurations. It supports loading V2Ray shared libraries (DLL on Windows, SO on Linux),
    managing proxy settings, testing multiple configurations for connectivity and latency,
    and pinging servers to measure latency.
    """
    def __init__(self, http_port=2300, socks_port=2301):
        """
        Initialize the V2ROOT instance with specified HTTP and SOCKS ports.

        Args:
            http_port (int, optional): The port for HTTP proxy. Defaults to 2300.
            socks_port (int, optional): The port for SOCKS proxy. Defaults to 2301.

        Raises:
            ValueError: If http_port or socks_port are not valid port numbers (1-65535).
            TypeError: If http_port or socks_port are not integers.
            OSError: If the platform is not Windows or Linux.
            FileNotFoundError: If the V2Ray shared library or executable is not found.
            OSError: If the shared library fails to load due to missing dependencies.
            RuntimeError: If V2Ray is not installed on Linux or the executable is invalid.
        """
        if not isinstance(http_port, int):
            raise TypeError("http_port must be an integer")
        if not isinstance(socks_port, int):
            raise TypeError("socks_port must be an integer")
        if not (1 <= http_port <= 65535):
            raise ValueError("http_port must be between 1 and 65535")
        if not (1 <= socks_port <= 65535):
            raise ValueError("socks_port must be between 1 and 65535")
        if http_port == socks_port:
            raise ValueError("http_port and socks_port must be different")

        if platform.system() == "Windows":
            lib_name = "libv2root.dll"
            v2ray_name = "v2ray.exe"
            build_dir = "build_win"
        elif platform.system() == "Linux":
            lib_name = "libv2root.so"
            v2ray_name = "v2ray"
            build_dir = "build_linux"
        else:
            raise OSError("Unsupported platform. V2Root currently supports Windows and Linux only.")

        script_dir = os.path.dirname(os.path.abspath(__file__))
        lib_path = os.path.join(script_dir, "lib", build_dir, lib_name)
        v2ray_path = os.path.join(script_dir, "lib", v2ray_name)

        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"Could not find {lib_name} at {lib_path}. Make sure it is built and placed in the correct directory.")
        if not os.path.exists(v2ray_path):
            raise FileNotFoundError(f"Could not find {v2ray_name} at {v2ray_path}. Please install V2Ray or place the executable in the correct directory.")

        # Check if v2ray is executable and has required dependencies
        if platform.system() == "Linux":
            if not os.access(v2ray_path, os.X_OK):
                raise RuntimeError(f"{v2ray_name} at {v2ray_path} is not executable. Ensure it has execute permissions (chmod +x {v2ray_path}).")
            try:
                result = subprocess.run(["ldd", v2ray_path], capture_output=True, text=True)
                if "not found" in result.stdout:
                    raise RuntimeError(f"{v2ray_name} at {v2ray_path} is missing dependencies. Run 'ldd {v2ray_path}' to check and install missing libraries (e.g., sudo apt install libjansson-dev libssl-dev).")
                # Test if v2ray is functional by running a simple command
                result = subprocess.run([v2ray_path, "--version"], capture_output=True, text=True, timeout=5)
                if result.returncode != 0:
                    raise RuntimeError(f"{v2ray_name} at {v2ray_path} is not functional. Running '{v2ray_path} --version' failed with error: {result.stderr}. Ensure V2Ray is properly installed and configured.")
            except subprocess.CalledProcessError:
                raise RuntimeError(f"Failed to check dependencies for {v2ray_name}. Ensure 'ldd' is available and {v2ray_path} is valid.")
            except subprocess.TimeoutExpired:
                raise RuntimeError(f"{v2ray_name} at {v2ray_path} is not responding. Ensure V2Ray is properly installed and functional.")
            except FileNotFoundError:
                raise RuntimeError(f"{v2ray_name} at {v2ray_path} cannot be executed. Ensure V2Ray is properly installed and the path is correct.")

        if platform.system() == "Windows":
            dll_dir = os.path.dirname(lib_path)
            os.add_dll_directory(dll_dir)
            os.environ["PATH"] = dll_dir + os.pathsep + os.environ.get("PATH", "")

        try:
            self.lib = ctypes.CDLL(lib_path)
        except OSError as e:
            raise OSError(f"Failed to load {lib_name}: {str(e)}. Ensure all dependencies (libjansson-4.dll, libssl-1_1-x64.dll, libcrypto-1_1-x64.dll) are in {os.path.dirname(lib_path)}")

        self.http_port = http_port
        self.socks_port = socks_port
        self.is_linux = platform.system() == "Linux"
        self.is_initialized = False  # Flag to track initialization status

        self.lib.init_v2ray.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self.lib.init_v2ray.restype = ctypes.c_int
        self.lib.reset_network_proxy.argtypes = []
        self.lib.reset_network_proxy.restype = ctypes.c_int
        self.lib.parse_config_string.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
        self.lib.parse_config_string.restype = ctypes.c_int
        self.lib.start_v2ray.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.start_v2ray.restype = ctypes.c_int
        self.lib.stop_v2ray.argtypes = []
        self.lib.stop_v2ray.restype = ctypes.c_int
        self.lib.test_config_connection.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int), ctypes.c_int, ctypes.c_int]
        self.lib.test_config_connection.restype = ctypes.c_int
        self.lib.ping_server.argtypes = [ctypes.c_char_p, ctypes.c_int]
        self.lib.ping_server.restype = ctypes.c_int

        self._init_v2ray('config.json', v2ray_path)
        print(f"{Fore.GREEN}V2ROOT initialized successfully{Style.RESET_ALL}")

    def _explain_error_code(self, error_code, context=""):
        """
        Explain the meaning of V2Ray error codes.

        Args:
            error_code (int): The error code returned by V2Ray functions.
            context (str): Additional context for the error (e.g., function name).

        Returns:
            str: A human-readable explanation of the error code.
        """
        error_codes = {
            -1: "General failure (e.g., file operation failed, fork failed, or system command error).",
            -2: "Service operation failed (e.g., V2Ray failed to start or connect).",
            -3: "Invalid configuration (e.g., malformed config string or file).",
            -4: "Network error (e.g., unable to connect to server or bind to port).",
            -5: "Initialization error (e.g., failed to load V2Ray core or dependencies).",
            -6: "Proxy setting error (e.g., failed to set or unset system proxy).",
        }
        default_message = f"Unknown error code: {error_code}. Check logs for details."
        return f"{context}: {error_codes.get(error_code, default_message)}"

    def _init_v2ray(self, config_file, v2ray_path):
        """
        Initialize the V2Ray core with a configuration file and V2Ray executable path.

        Args:
            config_file (str): Path to the V2Ray configuration file.
            v2ray_path (str): Path to the V2Ray executable.

        Raises:
            Exception: If initialization fails with a non-zero error code.
        """
        result = self.lib.init_v2ray(config_file.encode('utf-8'), v2ray_path.encode('utf-8'))
        if result != 0:
            error_message = self._explain_error_code(result, "Failed to initialize V2ROOT")
            if result == -1 and self.is_linux:
                error_message += f"\nThis may be due to V2Ray not being properly installed or configured at {v2ray_path}. Please ensure V2Ray is installed (e.g., sudo apt install v2ray) and the executable is functional."
            raise Exception(error_message)
        self.is_initialized = True  # Set the flag on successful initialization
        
    def reset_network_proxy(self):
        """
        Reset system network proxy settings.

        Raises:
            Exception: If resetting the proxy settings fails.
        """
        try:
            self.stop()
        except:
            pass
        result = self.lib.reset_network_proxy()
        if result != 0:
            raise Exception(self._explain_error_code(result, "Failed to reset network proxy"))
        print(f"{Fore.GREEN}Network settings reset successfully!{Style.RESET_ALL}")

    def set_config_string(self, config_str):
        """
        Parse and set a V2Ray configuration string.

        Args:
            config_str (str): V2Ray configuration string (e.g., VLESS, VMess).

        Raises:
            TypeError: If config_str is not a string.
            ValueError: If config_str is empty.
            Exception: If parsing the configuration string fails.
        """
        if not isinstance(config_str, str):
            raise TypeError("config_str must be a string")
        if not config_str.strip():
            raise ValueError("config_str cannot be empty")

        result = self.lib.parse_config_string(config_str.encode('utf-8'), self.http_port, self.socks_port)
        if result != 0:
            raise Exception(self._explain_error_code(result, "Failed to parse config string"))
        print(f"{Fore.GREEN}Connection OK{Style.RESET_ALL}")

    def start(self):
        """
        Start the V2Ray proxy service.

        On Linux, displays instructions for manually setting proxy environment variables
        and waits for user input to continue.

        Returns:
            int: The process ID (PID) of the started V2Ray process.

        Raises:
            Exception: If starting the V2Ray service fails.
        """

        pid = self.lib.start_v2ray(self.http_port, self.socks_port)
        if pid < 0:
            raise Exception(self._explain_error_code(pid, "Failed to start V2Ray"))
        print(f"{Fore.GREEN}V2Ray started successfully with PID: {pid}{Style.RESET_ALL}")

        if self.is_linux:
            print(f"{Fore.YELLOW}============================================================{Style.RESET_ALL}")
            print(f"{Fore.CYAN}V2Ray service is running! You need to manually set the proxy settings.{Style.RESET_ALL}")
            print(f"{Fore.CYAN}To set the proxy, run the following commands in your terminal:{Style.RESET_ALL}")
            print(f"{Fore.GREEN}export http_proxy=http://127.0.0.1:{self.http_port}{Style.RESET_ALL}")
            print(f"{Fore.GREEN}export https_proxy=http://127.0.0.1:{self.http_port}{Style.RESET_ALL}")
            print(f"{Fore.GREEN}export HTTP_PROXY=http://127.0.0.1:{self.http_port}{Style.RESET_ALL}")
            print(f"{Fore.GREEN}export HTTPS_PROXY=http://127.0.0.1:{self.http_port}{Style.RESET_ALL}")
            print(f"{Fore.GREEN}export socks_proxy=socks5://127.0.0.1:{self.socks_port}{Style.RESET_ALL}")
            print(f"{Fore.GREEN}export SOCKS_PROXY=socks5://127.0.0.1:{self.socks_port}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}To unset the proxy, run:{Style.RESET_ALL}")
            print(f"{Fore.GREEN}unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY socks_proxy SOCKS_PROXY{Style.RESET_ALL}")
            print(f"{Fore.YELLOW}============================================================{Style.RESET_ALL}")
            print(f"{Fore.MAGENTA}Press any key to continue...{Style.RESET_ALL}")

            rlist, _, _ = select.select([sys.stdin], [], [], None)
            if rlist:
                sys.stdin.readline()

        return pid

    def stop(self):
        """
        Stop the V2Ray proxy service.

        On Linux, displays instructions for unsetting proxy environment variables
        and waits for user input to continue.

        Raises:
            Exception: If stopping the V2Ray service fails.
        """
        result = self.lib.stop_v2ray()
        if result != 0:
            raise Exception(self._explain_error_code(result, "Failed to stop V2Ray"))
        print(f"{Fore.GREEN}V2Ray stopped successfully!{Style.RESET_ALL}")

        if self.is_linux:
            print(f"{Fore.YELLOW}============================================================{Style.RESET_ALL}")
            print(f"{Fore.CYAN}V2Ray service has stopped. Please unset the proxy settings.{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Run the following command in your terminal:{Style.RESET_ALL}")
            print(f"{Fore.GREEN}unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY socks_proxy SOCKS_PROXY{Style.RESET_ALL}")
            print(f"{Fore.YELLOW}============================================================{Style.RESET_ALL}")
            print(f"{Fore.MAGENTA}Press any key to continue...{Style.RESET_ALL}")

            rlist, _, _ = select.select([sys.stdin], [], [], None)
            if rlist:
                sys.stdin.readline()

    def test_connection(self, config_str):
        """
        Test connectivity and latency of a V2Ray configuration.

        Args:
            config_str (str): V2Ray configuration string to test.

        Returns:
            int: Latency of the connection in milliseconds.

        Raises:
            TypeError: If config_str is not a string.
            ValueError: If config_str is empty.
            Exception: If the connection test fails or V2Ray is not initialized.
        """
        if not isinstance(config_str, str):
            raise TypeError("config_str must be a string")
        if not config_str.strip():
            raise ValueError("config_str cannot be empty")
        if not self.is_initialized:
            raise Exception("V2Ray is not properly initialized. Ensure V2Ray is installed and configured correctly.")

        latency = ctypes.c_int()
        result = self.lib.test_config_connection(config_str.encode('utf-8'), ctypes.byref(latency), self.http_port, self.socks_port)
        if result != 0:
            raise Exception(self._explain_error_code(result, "Connection test failed"))
        print(f"{Fore.GREEN}Connection OK, Latency {latency.value}ms{Style.RESET_ALL}")
        return latency.value

    def ping_server(self, address, port):
        """
        Ping a server to measure latency.

        Args:
            address (str): The server address (IP or hostname) to ping.
            port (int): The port to connect to.

        Returns:
            int: Latency in milliseconds on success.

        Raises:
            TypeError: If address is not a string or port is not an integer.
            ValueError: If address is empty or port is not between 1 and 65535.
            Exception: If the ping fails (e.g., due to connection errors or invalid address).
        """
        if not isinstance(address, str):
            raise TypeError("address must be a string")
        if not isinstance(port, int):
            raise TypeError("port must be an integer")
        if not address.strip():
            raise ValueError("address cannot be empty")
        if not (1 <= port <= 65535):
            raise ValueError("port must be between 1 and 65535")

        result = self.lib.ping_server(address.encode('utf-8'), port)
        if result < 0:
            raise Exception(self._explain_error_code(result, f"Failed to ping server {address}:{port}"))
        print(f"{Fore.GREEN}Ping successful, Latency: {result}ms{Style.RESET_ALL}")
        return result

    def test_configs(self, config_input, output_file="goods.txt", min_latency=100, max_latency=800):
        """
        Test multiple V2Ray configurations and save valid ones to a file.

        Tests configurations from a text file or list, saves those with latency
        between min_latency and max_latency to the output file.

        Args:
            config_input (str or list): Path to a .txt file or a list of configuration strings.
            output_file (str, optional): Path to save valid configurations. Defaults to "goods.txt".
            min_latency (int, optional): Minimum acceptable latency in milliseconds. Defaults to 100.
            max_latency (int, optional): Maximum acceptable latency in milliseconds. Defaults to 800.

        Returns:
            str or None: The configuration with the lowest latency, or None if no valid configurations are found.

        Raises:
            TypeError: If config_input is not a string or list, or output_file is not a string,
                       or min_latency/max_latency are not numbers.
            ValueError: If config_input is empty, output_file is empty,
                        or min_latency/max_latency are invalid (negative or min > max).
            FileNotFoundError: If the input file does not exist.
        """
        if not isinstance(config_input, (str, list)):
            raise TypeError("config_input must be a string (file path) or a list of configs")
        if not isinstance(output_file, str):
            raise TypeError("output_file must be a string")
        if not output_file.strip():
            raise ValueError("output_file cannot be empty")
        if not isinstance(min_latency, (int, float)):
            raise TypeError("min_latency must be a number")
        if not isinstance(max_latency, (int, float)):
            raise TypeError("max_latency must be a number")
        if min_latency < 0:
            raise ValueError("min_latency cannot be negative")
        if max_latency < 0:
            raise ValueError("max_latency cannot be negative")
        if min_latency > max_latency:
            raise ValueError("min_latency cannot be greater than max_latency")

        if isinstance(config_input, str):
            if not os.path.exists(config_input):
                raise FileNotFoundError(f"Config file {config_input} not found")
            if not config_input.lower().endswith('.txt'):
                raise ValueError("Config file must be a .txt file")
            with open(config_input, 'r', encoding='utf-8') as f:
                configs = [line.strip() for line in f if line.strip()]
        else:
            configs = [config.strip() for config in config_input if config.strip()]

        total_configs = len(configs)
        if total_configs == 0:
            raise ValueError("No valid configs found")

        estimated_time = total_configs * 5
        minutes = estimated_time // 60
        seconds = estimated_time % 60

        print(f"{Fore.CYAN}Loaded {total_configs} configs{Style.RESET_ALL}")
        print(f"{Fore.CYAN}Estimated time: {minutes} minutes and {seconds} seconds{Style.RESET_ALL}")

        if platform.system() == "Windows":
            print(f"{Fore.RED}WARNING: Due to frequent starting and stopping of the proxy, we recommend avoiding system usage during the test to prevent proxy setting disruptions.{Style.RESET_ALL}")

        valid_configs = []
        for i, config in enumerate(configs, 1):
            print(f"{Fore.CYAN}Testing config {i}/{total_configs}: {config[:50]}...{Style.RESET_ALL}")
            try:
                latency = ctypes.c_int()
                result = self.lib.test_config_connection(config.encode('utf-8'), ctypes.byref(latency), self.http_port, self.socks_port)
                if result == 0 and min_latency <= latency.value <= max_latency:
                    valid_configs.append((config, latency.value))
                    print(f"{Fore.GREEN}Config {i} OK, Latency: {latency.value}ms{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}Config {i} failed: {self._explain_error_code(result, 'Test connection')}{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.RED}Config {i} failed: {str(e)}{Style.RESET_ALL}")
                continue

        with open(output_file, 'w', encoding='utf-8') as f:
            for config, _ in valid_configs:
                f.write(config + '\n')

        best_config = None
        if valid_configs:
            best_config = min(valid_configs, key=lambda x: x[1])[0]
            print(f"{Fore.GREEN}Best config: {best_config[:50]}... with latency {min(valid_configs, key=lambda x: x[1])[1]}ms{Style.RESET_ALL}")

        print(f"{Fore.GREEN}Test completed. Valid configs saved to {output_file}{Style.RESET_ALL}")
        return best_config