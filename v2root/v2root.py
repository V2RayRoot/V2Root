import ctypes
import os
from colorama import init, Fore, Style

init(autoreset=True)

class V2ROOT:
    def __init__(self, lib_path=None):
        """
        Initializes the V2ROOT class by loading the v2ray DLL library.

        Parameters:
        - lib_path (str or None): Path to the v2ray DLL file. If None (default), the path is dynamically determined
                                  relative to the location of this file (e.g., "lib/libv2root.dll").

        Behavior:
        - If lib_path is None, calculates the path to "libv2root.dll" relative to this file’s directory and changes
          the working directory to ensure v2ray.exe is found.
        - Loads the DLL and sets up argument/return types for all functions based on the updated core.

        Raises:
        - FileNotFoundError: If the DLL file is not found at the resolved or provided path, with a user-friendly message.
        """
        print(f"{Fore.YELLOW}Warning: V2Root is currently in beta and under testing. If you encounter any issues or bugs, please report them by opening an issue on GitHub: https://github.com/V2RayRoot/V2Root/issues{Style.RESET_ALL}")

        try:
            if lib_path is None:
                base_path = os.path.dirname(os.path.abspath(__file__))
                lib_path = os.path.join(base_path, "lib", "libv2root.dll")
                lib_path = os.path.normpath(lib_path)
                os.chdir(base_path)

            if not os.path.exists(lib_path):
                raise FileNotFoundError(f"Library {lib_path} not found")

            self.lib = ctypes.CDLL(lib_path)

            self.lib.load_v2ray_config.argtypes = []
            self.lib.load_v2ray_config.restype = ctypes.c_int

            self.lib.start_v2ray.argtypes = []
            self.lib.start_v2ray.restype = ctypes.c_int

            self.lib.stop_v2ray.argtypes = []
            self.lib.stop_v2ray.restype = ctypes.c_int

            self.lib.ping_server.argtypes = [ctypes.c_char_p, ctypes.c_int]
            self.lib.ping_server.restype = ctypes.c_int

            self.lib.parse_vless_string.argtypes = [ctypes.c_char_p]
            self.lib.parse_vless_string.restype = ctypes.c_int

            self.lib.check_server_status.argtypes = [ctypes.c_char_p]
            self.lib.check_server_status.restype = ctypes.c_int

            self.lib.ResetNetworkProxy.argtypes = []
            self.lib.ResetNetworkProxy.restype = None

            print(f"{Fore.GREEN}V2Root initialized successfully!{Style.RESET_ALL}")

        except FileNotFoundError as e:
            print(f"{Fore.RED}Error: Could not find the required DLL file at {lib_path}.{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Please download the DLL from the following link and place it in the 'lib' folder:")
            print(f"{Fore.CYAN}Download Link: https://your-link-to-dll.com/libv2root.dll")
            print(f"{Fore.CYAN}Alternatively, you can compile the DLL yourself using the source code (v2root_core.c):")
            print(f"{Fore.CYAN}1. Ensure you have MinGW-w64 installed. If not, download and install it:")
            print(f"{Fore.CYAN}   - For Windows, install MSYS2 from https://www.msys2.org/")
            print(f"{Fore.CYAN}   - Open MSYS2 terminal and install MinGW-w64:")
            print(f"{Fore.CYAN}     pacman -S mingw-w64-x86_64-gcc")
            print(f"{Fore.CYAN}2. Navigate to the directory containing v2root_core.c.")
            print(f"{Fore.CYAN}3. Compile the DLL using the following command:")
            print(f"{Fore.CYAN}   x86_64-w64-mingw32-gcc -shared -o libv2root.dll v2root_core.c -lws2_32 -lwininet")
            print(f"{Fore.CYAN}4. Place the generated libv2root.dll in the 'lib' folder.")
            print(f"{Fore.CYAN}After completing these steps, try running the program again.{Style.RESET_ALL}")
            raise

        except Exception as e:
            print(f"{Fore.RED}Error: Failed to initialize V2Root: {str(e)}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Ensure that all dependencies are installed and the DLL is compatible with your system.")
            print(f"{Fore.CYAN}If the problem persists, contact support or check the documentation for more details.{Style.RESET_ALL}")
            raise

    def load_config(self):
        """
        Loads the v2ray configuration file as defined in the core library.

        Behavior:
        - Calls the load_v2ray_config function from the DLL to validate the configuration file.

        Raises:
        - Exception: If the configuration cannot be loaded (e.g., file is missing or inaccessible), with a user-friendly message.
        """
        try:
            result = self.lib.load_v2ray_config()
            if result != 0:
                raise Exception("Failed to load config")

        except Exception as e:
            print(f"{Fore.RED}Error: Could not load the configuration: {str(e)}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Ensure that the required configuration file exists in the current working directory.")
            print(f"{Fore.CYAN}You may need to generate it first using set_vless_string() or verify its location.")
            print(f"{Fore.CYAN}If the issue persists, try resetting the network settings using client.reset_network_proxy() and start over.{Style.RESET_ALL}")
            raise

    def start(self):
        """
        Starts the v2ray process using the loaded configuration.

        Behavior:
        - Calls the start_v2ray function from the DLL.

        Returns:
        - int: The process ID (PID) of the started v2ray instance.

        Raises:
        - Exception: If v2ray fails to start (-1) or is already running (-2), with a user-friendly message.
        """
        try:
            pid = self.lib.start_v2ray()
            if pid == -1:
                raise Exception("Failed to start v2ray")
            elif pid == -2:
                raise Exception("v2ray is already running")
            print(f"{Fore.GREEN}v2ray started successfully with PID: {pid}{Style.RESET_ALL}")
            return pid

        except Exception as e:
            if str(e) == "v2ray is already running":
                print(f"{Fore.YELLOW}Notice: v2ray is already running on your system.{Style.RESET_ALL}")
                print(f"{Fore.CYAN}Solution: If you want to restart v2ray or are experiencing issues (e.g., connection problems),")
                print(f"{Fore.CYAN}you can reset all settings and stop the current process by running:")
                print(f"{Fore.CYAN}   client.reset_network_proxy()")
                print(f"{Fore.CYAN}Then try starting v2ray again with client.start().{Style.RESET_ALL}")
            else:
                print(f"{Fore.RED}Error: Could not start v2ray: {str(e)}{Style.RESET_ALL}")
                print(f"{Fore.CYAN}Solution: Ensure that v2ray.exe is present in the 'lib' folder and is accessible.")
                print(f"{Fore.CYAN}You may also need to reset the network settings using client.reset_network_proxy() and try again.")
                print(f"{Fore.CYAN}If the issue persists, check if another process is using the required ports (2300, 2301).{Style.RESET_ALL}")
            raise

    def stop(self):
        """
        Stops the currently running v2ray process.

        Behavior:
        - Calls the stop_v2ray function from the DLL.

        Notes:
        - Prints a success message if the process is stopped or no process is running.
        - Provides guidance if the operation fails.
        """
        try:
            result = self.lib.stop_v2ray()
            if result == 0:
                print(f"{Fore.GREEN}v2ray stopped successfully!{Style.RESET_ALL}")
            else:
                print(f"{Fore.YELLOW}Notice: No active v2ray process was found, or it could not be stopped.{Style.RESET_ALL}")
                print(f"{Fore.CYAN}Solution: This might happen if v2ray was already stopped or the process ID is invalid.")
                print(f"{Fore.CYAN}To ensure a clean state, you can run client.reset_network_proxy() to reset all settings.{Style.RESET_ALL}")

        except Exception as e:
            print(f"{Fore.RED}Error: An unexpected error occurred while stopping v2ray: {str(e)}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Try resetting all settings using client.reset_network_proxy() to ensure a clean state.")
            print(f"{Fore.CYAN}If the issue persists, check the Task Manager to manually stop any v2ray.exe processes.{Style.RESET_ALL}")
            raise

    def ping(self, address, port):
        """
        Measures latency to a specified server address and port.

        Parameters:
        - address (str): The server IP address (e.g., "1.1.1.1").
        - port (int): The port to test (e.g., 443).

        Returns:
        - int: Latency in milliseconds.

        Raises:
        - Exception: If the ping fails (returns < 0), with a user-friendly message.
        """
        try:
            addr = address.encode('utf-8')
            result = self.lib.ping_server(addr, port)
            if result < 0:
                raise Exception("Ping failed")
            print(f"{Fore.GREEN}Ping successful! Latency to {address}:{port} is {result}ms.{Style.RESET_ALL}")
            return result

        except Exception as e:
            print(f"{Fore.RED}Error: Failed to ping the server {address}:{port}: {str(e)}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Check if the server address and port are correct.")
            print(f"{Fore.CYAN}Ensure that your network connection is stable and the server is reachable.")
            print(f"{Fore.CYAN}If you suspect proxy issues, try resetting the network settings with client.reset_network_proxy() and try again.{Style.RESET_ALL}")
            raise

    def set_vless_string(self, vless_str):
        """
        Parses a VLESS string and generates a v2ray configuration file as defined in the core library.

        Parameters:
        - vless_str (str): The VLESS string to parse (e.g., "vless://uuid@address:port?params").

        Behavior:
        - Encodes the VLESS string as UTF-8.
        - Calls parse_vless_string from the DLL to generate the configuration file.

        Raises:
        - Exception: If parsing fails (e.g., invalid VLESS string), with a user-friendly message.
        """
        try:
            vless = vless_str.encode('utf-8')
            result = self.lib.parse_vless_string(vless)
            if result != 0:
                raise Exception("Failed to parse VLESS string")

        except Exception as e:
            print(f"{Fore.RED}Error: Could not parse the VLESS string: {str(e)}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Ensure that the VLESS string is in the correct format (e.g., 'vless://uuid@address:port?params').")
            print(f"{Fore.CYAN}Check for typos or missing parameters in the string.")
            print(f"{Fore.CYAN}If the issue persists, reset the network settings with client.reset_network_proxy() and try again.{Style.RESET_ALL}")
            raise
        
    def check_server_status(self, vless_str):
        """
        Checks the connection status of a VLESS server and returns the latency.

        Parameters:
        - vless_str (str): The VLESS string to test (e.g., "vless://uuid@address:port?params").

        Returns:
        - int: Latency in milliseconds if successful.

        Raises:
        - Exception: If the check fails, with a user-friendly message based on the error code.
        """
        try:
            vless = vless_str.encode('utf-8')
            result = self.lib.check_server_status(vless)
            if result < 0:
                error_messages = {
                    -1: "Invalid input (check VLESS string format)",
                    -2: "WSAStartup failed (network initialization error)",
                    -3: "Socket creation failed",
                    -4: "DNS resolution failed (check server address or DNS settings)",
                    -5: "Connection timed out (server may be offline)"
                }
                raise Exception(error_messages.get(result, "Unknown error"))
            print(f"{Fore.GREEN}Server latency: {result}ms{Style.RESET_ALL}")
            return result

        except Exception as e:
            print(f"{Fore.RED}Error checking server status: {str(e)}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Ensure the VLESS string is correct and the server is reachable.")
            print(f"{Fore.CYAN}If the issue persists, reset network settings with client.reset_network_proxy() and try again.{Style.RESET_ALL}")
            raise

    def reset_network_proxy(self):
        """
        Resets all network proxy settings and system changes made by V2Root, restoring the system to its initial state.

        Behavior:
        - Calls the ResetNetworkProxy function from the DLL, which performs the following:
        - Stops the v2ray process if it is running.
        - Disables the system proxy by setting ProxyEnable to 0 in the registry.
        - Clears all proxy configurations by deleting ProxyServer, ProxyOverride, and AutoConfigURL from the registry.
        - Removes the V2Root registry key (HKEY_CURRENT_USER\\Software\\V2Root) to delete stored process information.
        - Notifies Windows to apply the changes immediately, ensuring the proxy settings are updated without requiring a restart.

        Notes:
        - This function ensures that all modifications made by V2Root, including the running v2ray process, proxy settings, 
        and registry entries, are fully reverted to their initial state.
        - Requires appropriate permissions to modify the Windows registry; running with administrator privileges is recommended.
        """
        try:
            self.lib.ResetNetworkProxy()
            print(f"{Fore.GREEN}Network settings and V2Root changes reset successfully!{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Note: All proxy settings, running v2ray processes, and registry entries have been reset.")
            print(f"{Fore.CYAN}You can now start a new v2ray session with client.start() if needed.{Style.RESET_ALL}")

        except Exception as e:
            print(f"{Fore.RED}Error: Failed to reset network settings: {str(e)}{Style.RESET_ALL}")
            print(f"{Fore.CYAN}Solution: Ensure the program has administrator privileges to modify the Windows registry.")
            print(f"{Fore.CYAN}Run your script or application as an administrator:")
            print(f"{Fore.CYAN}  - Right-click on your terminal or script and select 'Run as administrator'.")
            print(f"{Fore.CYAN}If the issue persists, manually check the registry (HKEY_CURRENT_USER\\Software\\V2Root) and proxy settings.")
            print(f"{Fore.CYAN}You can also try restarting your system to ensure all changes are applied.{Style.RESET_ALL}")
            raise