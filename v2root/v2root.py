from colorama import init, Fore, Style
from .logger import logger, log_function_call, configure_logger
import ctypes
import os
import sys
import platform
import subprocess
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
    @log_function_call(log_args=True)
    def __init__(self, http_port=2300, socks_port=2301, v2ray_path=None, lib_path=None):
        """
        Initialize the V2ROOT instance.

        Args:
            http_port (int): The HTTP proxy port to use (default: 2300)
            socks_port (int): The SOCKS proxy port to use (default: 2301)
            v2ray_path (str): Path to V2Ray executable (optional, default: None)
            lib_path (str): Path to the libv2root shared library file (optional, default: None)
                            If not provided, the default system paths will be searched.

        Raises:
            ValueError: If http_port or socks_port are not valid port numbers (1-65535).
            TypeError: If http_port or socks_port are not integers, or v2ray_path is not a string.
            OSError: If the platform is not Windows or Linux.
            FileNotFoundError: If the V2Ray shared library or executable is not found.
            RuntimeError: On Windows if v2ray_path is not provided or invalid.
                         On Linux if V2Ray is not installed via package manager.
        """
        if not isinstance(http_port, int):
            raise TypeError("http_port must be an integer")
        if not isinstance(socks_port, int):
            raise TypeError("socks_port must be an integer")
        if v2ray_path is not None and not isinstance(v2ray_path, str):
            raise TypeError("v2ray_path must be a string or None")
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
        lib_path_default = os.path.join(script_dir, "lib", build_dir, lib_name)
        
        
        v2ray_path_resolved = None
        
        if platform.system() == "Windows":
            
            if not v2ray_path:
                error_msg = (
                    f"{Fore.RED}ERROR: v2ray_path is required on Windows!{Style.RESET_ALL}\n\n"
                    f"Please provide the path to v2ray.exe when initializing V2ROOT:\n"
                    f"  {Fore.YELLOW}proxy = V2ROOT(v2ray_path='C:\\\\path\\\\to\\\\v2ray.exe'){Style.RESET_ALL}\n\n"
                    f"Download V2Ray from: {Fore.CYAN}https://github.com/v2fly/v2ray-core/releases{Style.RESET_ALL}\n"
                    f"Extract and provide the full path to v2ray.exe"
                )
                logger.error("v2ray_path not provided on Windows")
                raise RuntimeError(error_msg)
            
            if not os.path.exists(v2ray_path):
                error_msg = (
                    f"{Fore.RED}ERROR: V2Ray executable not found at: {v2ray_path}{Style.RESET_ALL}\n\n"
                    f"Please ensure:\n"
                    f"  1. The path is correct\n"
                    f"  2. v2ray.exe exists at the specified location\n"
                    f"  3. You have read permissions for the file\n\n"
                    f"Download V2Ray from: {Fore.CYAN}https://github.com/v2fly/v2ray-core/releases{Style.RESET_ALL}"
                )
                logger.error(f"v2ray_path does not exist: {v2ray_path}")
                raise FileNotFoundError(error_msg)
            
            v2ray_path_resolved = v2ray_path
            logger.info(f"Using user-provided V2Ray path on Windows: {v2ray_path}")
            print(f"{Fore.GREEN}V2Ray executable found: {v2ray_path}{Style.RESET_ALL}")
            
        else:
            
            if v2ray_path:
                logger.warning(f"v2ray_path ignored on Linux (provided: {v2ray_path}). Using system-installed V2Ray.")
                print(f"{Fore.YELLOW}Note: v2ray_path is ignored on Linux. Using system-installed V2Ray.{Style.RESET_ALL}")
            
            
            system_paths = [
                "/usr/bin/v2ray",
                "/usr/local/bin/v2ray"
            ]
            
            for path in system_paths:
                if os.path.exists(path):
                    v2ray_path_resolved = path
                    logger.info(f"Found system-installed V2Ray at: {path}")
                    break
            
            
            if not v2ray_path_resolved:
                try:
                    result = subprocess.run(["which", "v2ray"], capture_output=True, text=True, timeout=5)
                    if result.returncode == 0 and result.stdout.strip():
                        v2ray_path_resolved = result.stdout.strip()
                        logger.info(f"Found V2Ray using which command: {v2ray_path_resolved}")
                except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
                    pass
            
            
            if not v2ray_path_resolved:
                error_msg = (
                    f"{Fore.RED}ERROR: V2Ray is not installed on this system!{Style.RESET_ALL}\n\n"
                    f"V2ROOT requires V2Ray to be installed via your package manager.\n\n"
                    f"Installation instructions:\n"
                    f"  {Fore.YELLOW}Debian/Ubuntu:{Style.RESET_ALL}\n"
                    f"    sudo apt update\n"
                    f"    sudo apt install v2ray\n\n"
                    f"  {Fore.YELLOW}Fedora/RHEL/CentOS:{Style.RESET_ALL}\n"
                    f"    sudo dnf install v2ray\n"
                    f"    # or\n"
                    f"    sudo yum install v2ray\n\n"
                    f"  {Fore.YELLOW}Arch Linux:{Style.RESET_ALL}\n"
                    f"    sudo pacman -S v2ray\n\n"
                    f"  {Fore.YELLOW}Manual installation:{Style.RESET_ALL}\n"
                    f"    bash <(curl -L https://raw.githubusercontent.com/v2fly/fhs-install-v2ray/master/install-release.sh)\n\n"
                    f"After installation, V2Ray should be available as 'v2ray' in your PATH.\n"
                    f"Verify installation: {Fore.CYAN}v2ray version{Style.RESET_ALL}"
                )
                logger.error("V2Ray not found on Linux system")
                raise RuntimeError(error_msg)
            
            print(f"{Fore.GREEN}Using system-installed V2Ray: {v2ray_path_resolved}{Style.RESET_ALL}")

        if not os.path.exists(lib_path_default):
            raise FileNotFoundError(f"Could not find {lib_name} at {lib_path_default}. Make sure it is built and placed in the correct directory.")

        
        if platform.system() == "Linux":
            if not os.access(v2ray_path_resolved, os.X_OK):
                raise RuntimeError(f"{v2ray_name} at {v2ray_path_resolved} is not executable. Ensure it has execute permissions (chmod +x {v2ray_path_resolved}).")
            try:
                result = subprocess.run(["ldd", v2ray_path_resolved], capture_output=True, text=True)
                if "not found" in result.stdout:
                    raise RuntimeError(f"{v2ray_name} at {v2ray_path_resolved} is missing dependencies. Run 'ldd {v2ray_path_resolved}' to check and install missing libraries.")
                result = subprocess.run([v2ray_path_resolved, "--version"], capture_output=True, text=True, timeout=5)
                if result.returncode != 0:
                    raise RuntimeError(f"{v2ray_name} at {v2ray_path_resolved} is not functional. Running '{v2ray_path_resolved} --version' failed: {result.stderr}")
            except subprocess.CalledProcessError:
                raise RuntimeError(f"Failed to check dependencies for {v2ray_name}.")
            except subprocess.TimeoutExpired:
                raise RuntimeError(f"{v2ray_name} at {v2ray_path_resolved} is not responding.")
            except FileNotFoundError:
                raise RuntimeError(f"{v2ray_name} at {v2ray_path_resolved} cannot be executed.")
        else:
            
            try:
                result = subprocess.run([v2ray_path_resolved, "version"], capture_output=True, text=True, timeout=5, shell=True)
                if result.returncode != 0:
                    logger.warning(f"V2Ray validation warning: {result.stderr}")
            except Exception as e:
                logger.warning(f"Could not validate V2Ray executable: {str(e)}")

        if platform.system() == "Windows":
            dll_dir = os.path.dirname(lib_path_default)
            os.add_dll_directory(dll_dir)
            os.environ["PATH"] = dll_dir + os.pathsep + os.environ.get("PATH", "")

        try:
            self.lib = ctypes.CDLL(lib_path_default)
            logger.debug(f"Successfully loaded library: {lib_path_default}")
        except OSError as e:
            error_msg = f"Failed to load {lib_name}: {str(e)}. Ensure all dependencies are in {os.path.dirname(lib_path_default)}"
            logger.error(error_msg)
            raise OSError(error_msg)

        self.http_port = http_port
        self.socks_port = socks_port
        self.is_linux = platform.system() == "Linux"
        self.is_initialized = False
        self.v2ray_path = v2ray_path_resolved

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

        self.lib.probe_config_quick.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p), ctypes.c_int, ctypes.c_int]
        self.lib.probe_config_quick.restype = ctypes.c_int
        self.lib.probe_config_full.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p), ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.lib.probe_config_full.restype = ctypes.c_int
        
        self.lib.measure_ttfb.argtypes = [ctypes.c_char_p, ctypes.c_int]
        self.lib.measure_ttfb.restype = ctypes.c_char_p  

        self._init_v2ray('config.json', v2ray_path_resolved)
        logger.info(f"V2ROOT initialized successfully with V2Ray at: {v2ray_path_resolved}")
        print(f"{Fore.GREEN}V2ROOT initialized successfully{Style.RESET_ALL}")

    def _explain_error_code(self, error_code, context=""):
        """
        Explain V2Ray error codes in a clear, beginner-friendly way with short steps to fix issues.

        Args:
            error_code (int): The error code from V2Ray functions.
            context (str): What the program was trying to do when the error happened.

        Returns:
            str: A colorful, concise error message with simple fix instructions and a documentation link.
        """
        
        logger.error(f"Error code {error_code} encountered during: {context}")
        
        error_codes = {
            -1: (
                f"{Fore.RED}General Error{Fore.RESET}",
                "Something went wrong, maybe a missing file or bad configuration string.",
                [
                    f"Check your config string (e.g., {Fore.YELLOW}vless://user-id@server:443{Fore.RESET}). Must start with {Fore.YELLOW}vless://, vmess://, or ss://{Fore.RESET}.",
                    f"On {Fore.YELLOW}Linux{Fore.RESET}, check V2Ray: {Fore.YELLOW}v2ray version{Fore.RESET}.",
                    f"On {Fore.YELLOW}Windows{Fore.RESET}, ensure {Fore.YELLOW}C:\\V2Root\\libv2root.dll{Fore.RESET} exists (use your path). Check: {Fore.YELLOW}dir C:\\V2Root\\libv2root.dll{Fore.RESET}.",
                    f"On {Fore.YELLOW}Linux{Fore.RESET}, ensure {Fore.YELLOW}/usr/local/lib/v2root/libv2root.so{Fore.RESET} exists: {Fore.YELLOW}ls /usr/local/lib/v2root/libv2root.so{Fore.RESET}.",
                    f"Run as admin: {Fore.YELLOW}Windows{Fore.RESET}: Right-click {Fore.YELLOW}v2root.py{Fore.RESET}, 'Run as administrator'. {Fore.YELLOW}Linux{Fore.RESET}: {Fore.YELLOW}sudo python3 v2root.py{Fore.RESET}.",
                    f"Check {Fore.CYAN}v2root.log{Fore.RESET} for details."
                ],
                "More: https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Error_-1.md"
            ),
            -2: (
                f"{Fore.RED}Service Error{Fore.RESET}",
                "V2Ray failed to start or connect to the internet.",
                [
                    f"Verify config string (e.g., {Fore.YELLOW}vless://user-id@server:443{Fore.RESET}) starts with {Fore.YELLOW}vless://, vmess://, or ss://{Fore.RESET}.",
                    f"On {Fore.YELLOW}Linux{Fore.RESET}, check V2Ray: {Fore.YELLOW}v2ray version{Fore.RESET}.",
                    f"Check ports {self.http_port}, {self.socks_port}: {Fore.YELLOW}Linux{Fore.RESET}: {Fore.YELLOW}netstat -tuln | grep {self.http_port}{Fore.RESET}. {Fore.YELLOW}Windows{Fore.RESET}: {Fore.YELLOW}netstat -an | findstr {self.http_port}{Fore.RESET}.",
                    f"Test internet: {Fore.YELLOW}ping 8.8.8.8{Fore.RESET}.",
                    f"Check {Fore.CYAN}v2root.log{Fore.RESET} for clues."
                ],
                "More: https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Error_-2.md"
            ),
            -3: (
                f"{Fore.RED}Config Error{Fore.RESET}",
                "Your configuration string is invalid.",
                [
                    f"Ensure config string starts with {Fore.YELLOW}vless://, vmess://, or ss://{Fore.RESET} (e.g., {Fore.YELLOW}vless://user-id@server:443{Fore.RESET}).",
                    f"Check server address, port, ID. Ask VPN provider if unsure.",
                    f"Check {Fore.CYAN}v2root.log{Fore.RESET} for config errors."
                ],
                "More: https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Error_-3.md"
            ),
            -4: (
                f"{Fore.RED}Connection Error{Fore.RESET}",
                "Couldn’t connect to the server or network.",
                [
                    f"Verify server in config string (e.g., {Fore.YELLOW}vless://user-id@server:443{Fore.RESET}). Test: {Fore.YELLOW}ping <server>{Fore.RESET}.",
                    f"Check ports {self.http_port}, {self.socks_port}: {Fore.YELLOW}Linux{Fore.RESET}: {Fore.YELLOW}netstat -tuln | grep {self.http_port}{Fore.RESET}. {Fore.YELLOW}Windows{Fore.RESET}: {Fore.YELLOW}netstat -an | findstr {self.http_port}{Fore.RESET}.",
                    f"Allow V2Ray in firewall: {Fore.YELLOW}Linux{Fore.RESET}: {Fore.YELLOW}sudo ufw allow {self.http_port}{Fore.RESET}.",
                    f"Check {Fore.CYAN}v2root.log{Fore.RESET} for network errors."
                ],
                "More: https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Error_-4.md"
            ),
            -5: (
                f"{Fore.RED}Initialization Error{Fore.RESET}",
                "Couldn’t start V2Ray due to missing files.",
                [
                    f"On {Fore.YELLOW}Linux{Fore.RESET}, check V2Ray: {Fore.YELLOW}v2ray version{Fore.RESET}.",
                    f"On {Fore.YELLOW}Windows{Fore.RESET}, check {Fore.YELLOW}C:\\V2Root\\libv2root.dll{Fore.RESET}: {Fore.YELLOW}dir C:\\V2Root\\libv2root.dll{Fore.RESET}.",
                    f"On {Fore.YELLOW}Linux{Fore.RESET}, check {Fore.YELLOW}/usr/local/lib/v2root/libv2root.so{Fore.RESET}: {Fore.YELLOW}ls /usr/local/lib/v2root/libv2root.so{Fore.RESET}.",
                    f"Check {Fore.CYAN}v2root.log{Fore.RESET} for missing files."
                ],
                "More: https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Error_-5.md"
            ),
            -6: (
                f"{Fore.RED}Proxy Error{Fore.RESET}",
                "Couldn’t set or clear system proxy settings.",
                [
                    f"Run as admin: {Fore.YELLOW}Windows{Fore.RESET}: Right-click {Fore.YELLOW}v2root.py{Fore.RESET}, 'Run as administrator'. {Fore.YELLOW}Linux{Fore.RESET}: {Fore.YELLOW}sudo python3 v2root.py{Fore.RESET}.",
                    f"Close other VPN/proxy programs.",
                    f"Reset proxy: {Fore.YELLOW}Windows{Fore.RESET}: {Fore.YELLOW}netsh winhttp reset proxy{Fore.RESET}. {Fore.YELLOW}Linux{Fore.RESET}: {Fore.YELLOW}gsettings reset org.gnome.system.proxy{Fore.RESET}.",
                    f"Check {Fore.CYAN}v2root.log{Fore.RESET} for proxy errors."
                ],
                "More: https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Error_-6.md"
            ),
            -7:(
                f"{Fore.RED}Process Already Running{Fore.RESET}",
                "A V2Ray process is already running - cannot start a new one",
                [
                    f"Stop the existing V2Ray process first using {Fore.YELLOW}stop(){Fore.RESET}",
                    f"Check Task Manager (Windows) or process list (Linux) for v2ray",
                    f"Wait a few seconds and try again"
                ],
                "More: https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.2.0/Error_-7.md"
            )
        }

        error_info = error_codes.get(
            error_code,
            (
                f"{Fore.RED}Unknown Error (Code: {error_code}){Fore.RESET}",
                "Something went wrong, but we’re not sure what.",
                [
                    f"Check {Fore.CYAN}v2root.log{Fore.RESET} for details.",
                    f"On {Fore.YELLOW}Linux{Fore.RESET}, check V2Ray: {Fore.YELLOW}v2ray version{Fore.RESET}.",
                    f"On {Fore.YELLOW}Windows{Fore.RESET}, check {Fore.YELLOW}C:\\V2Root\\libv2root.dll{Fore.RESET}: {Fore.YELLOW}dir C:\\V2Root\\libv2root.dll{Fore.RESET}.",
                    f"Restart the program or contact support."
                ],
                "Report: https://github.com/V2RayRoot/V2Root/issues"
            )
        )

        script_file = os.path.basename(sys.argv[0])

        error_message = (
            f"\n{Fore.RED}=== Error ==={Fore.RESET}\n"
            f"{Fore.YELLOW}Issue:{Fore.RESET} {error_info[0]}\n"
            f"{Fore.YELLOW}Why:{Fore.RESET} {error_info[1]}\n"
            f"{Fore.YELLOW}Fix:{Fore.RESET}\n"
        )
        for i, step in enumerate(error_info[2], 1):
            error_message += f"  {i}. {step}\n"
        error_message += (
            f"\n{Fore.CYAN}Help:{Fore.RESET} See {Fore.CYAN}v2root.log{Fore.RESET}.\n"
            f"{Fore.CYAN}Still stuck?{Fore.RESET} Contact Telegram (@Sepehr0Day) or GitHub: {Fore.YELLOW}https://github.com/V2RayRoot/V2Root/issues{Fore.RESET}\n"
            f"Include: Script ({Fore.CYAN}{script_file}{Fore.RESET}), config string (hide sensitive parts), {Fore.CYAN}v2root.log{Fore.RESET}, OS.\n"
            f"{Fore.CYAN}More Details:{Fore.RESET} {error_info[3]}\n"
        )

        return error_message
    
    @log_function_call
    def _init_v2ray(self, config_file, v2ray_path):
        """
        Initialize the V2Ray core with a configuration file and V2Ray executable path.

        Args:
            config_file (str): Path to the V2Ray configuration file.
            v2ray_path (str): Path to the V2Ray executable.

        Raises:
            Exception: If initialization fails with a non-zero error code.
        """
        logger.debug(f"Initializing V2Ray with config: {config_file}, path: {v2ray_path}")
        result = self.lib.init_v2ray(config_file.encode('utf-8'), v2ray_path.encode('utf-8'))
        if result != 0:
            error_message = self._explain_error_code(result, "Failed to initialize V2ROOT")
            logger.error(f"V2Ray initialization failed with code {result}")
            if result == -1 and self.is_linux:
                extra_info = f"This may be due to V2Ray not being properly installed or configured at {v2ray_path}."
                logger.error(extra_info)
                error_message += f"\n{extra_info} Please ensure V2Ray is installed and the executable is functional."
            raise Exception(error_message)
        logger.info("V2Ray core initialized successfully")
        self.is_initialized = True 
        
    @log_function_call
    def reset_network_proxy(self):
        """
        Reset system network proxy settings.

        Raises:
            Exception: If resetting the proxy settings fails.
        """
        logger.info("Resetting network proxy settings")
        try:
            self.stop()
        except Exception as e:
            logger.warning(f"Failed to stop V2Ray before resetting proxy: {str(e)}")
            
        result = self.lib.reset_network_proxy()
        if result != 0:
            error_msg = self._explain_error_code(result, "Failed to reset network proxy")
            logger.error(f"Failed to reset network proxy: code {result}")
            raise Exception(error_msg)
        
        logger.info("Network settings reset successfully")
        print(f"{Fore.GREEN}Network settings reset successfully!{Style.RESET_ALL}")

    @log_function_call
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

        
        safe_config = config_str[:20] + "..." if len(config_str) > 20 else config_str
        logger.info(f"Setting configuration: {safe_config}")
        
        result = self.lib.parse_config_string(config_str.encode('utf-8'), self.http_port, self.socks_port)
        if result != 0:
            error_msg = self._explain_error_code(result, "Failed to parse config string")
            logger.error(f"Failed to parse config string: code {result}")
            raise Exception(error_msg)
        
        logger.info("Configuration applied successfully")
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
        logger.info(f"Starting V2Ray (HTTP port: {self.http_port}, SOCKS port: {self.socks_port})")
        result = self.lib.start_v2ray(self.http_port, self.socks_port)
        
        if result == -7:
            error_msg = self._explain_error_code(-7, "Failed to start V2Ray - already running")
            logger.error(f"V2Ray already running: code {result}")
            print(error_msg)
            raise Exception(error_msg)
        elif result < 0:
            error_msg = self._explain_error_code(result, "Failed to start V2Ray")
            logger.error(f"Failed to start V2Ray: code {result}")
            print(error_msg)
            raise Exception(error_msg)
        else:
            logger.info(f"V2Ray started successfully with PID: {result}")
            print(f"✅ V2Ray started successfully (PID: {result})")
            return result

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

        
        try:
            ttfb_result = self._measure_ttfb(config_str)
            if ttfb_result['success']:
                latency = ttfb_result['ttfb_ms']
                print(f"{Fore.GREEN}Connection OK, Latency {latency}ms{Style.RESET_ALL}")
                return latency
        except Exception as e:
            logger.debug(f"TTFB measurement failed, falling back to probe: {e}")
            
        
        probe_result = self._probe_full(config_str, attempts=1)
        
        if probe_result['success']:
            latency = probe_result['total_ms']
            print(f"{Fore.GREEN}Connection OK, Latency {latency}ms{Style.RESET_ALL}")
            return latency
        else:
            
            latency = ctypes.c_int()
            result = self.lib.test_config_connection(config_str.encode('utf-8'), ctypes.byref(latency), self.http_port, self.socks_port)
            if result != 0:
                raise Exception(self._explain_error_code(result, "Connection test failed"))
            print(f"{Fore.GREEN}Connection OK, Latency {latency.value}ms{Style.RESET_ALL}")
            return latency.value

    @log_function_call
    def _probe_quick(self, config_str):
        """
        [PRIVATE] Quick probe (DNS + TCP only) for fast batch testing.
        
        This internal method performs a lightweight connection test by only checking
        DNS resolution and TCP connection without HTTP requests.
        
        Args:
            config_str (str): V2Ray configuration string to probe.
            
        Returns:
            dict: Probe results with success status and latency info.
            
        Raises:
            TypeError: If config_str is not a string.
            ValueError: If config_str is empty.
            Exception: If V2Ray is not initialized or probe fails.
        """
        if not isinstance(config_str, str):
            raise TypeError("config_str must be a string")
        if not config_str.strip():
            raise ValueError("config_str cannot be empty")
        if not self.is_initialized:
            raise Exception("V2Ray is not properly initialized.")
        
        
        class ProbeResult(ctypes.Structure):
            _fields_ = [
                ("success", ctypes.c_int),
                ("total_ms", ctypes.c_int),
                ("dns_ms", ctypes.c_int),
                ("tcp_ms", ctypes.c_int),
                ("ttfb_ms", ctypes.c_int),
                ("score", ctypes.c_double),
                ("error_type", ctypes.c_char * 64)
            ]
        
        result_ptr = ctypes.pointer(ProbeResult())
        result = self.lib.probe_config_quick(
            config_str.encode('utf-8'),
            ctypes.cast(result_ptr, ctypes.POINTER(ctypes.c_void_p)),
            self.http_port,
            self.socks_port
        )
        
        if result != 0:
            error_msg = self._explain_error_code(result, "Quick probe failed")
            logger.error(f"Quick probe failed with code {result}")
            return {
                'success': False,
                'total_ms': -1,
                'dns_ms': -1,
                'tcp_ms': -1,
                'error_type': f"Error code: {result}. {error_msg}"
            }
        
        probe_result = result_ptr.contents
        return {
            'success': bool(probe_result.success),
            'total_ms': probe_result.total_ms,
            'dns_ms': probe_result.dns_ms,
            'tcp_ms': probe_result.tcp_ms,
            'error_type': probe_result.error_type.decode('utf-8') if not probe_result.success else None
        }
    
    @log_function_call
    def _probe_full(self, config_str, attempts=1):
        """
        [PRIVATE] Full probe (DNS + TCP + HTTP GET) for comprehensive testing.
        
        This internal method performs a complete end-to-end test similar to V2rayNG,
        measuring DNS resolution time, TCP connection time, and Time to First Byte (TTFB)
        via HTTP GET request.
        
        Args:
            config_str (str): V2Ray configuration string to probe.
            attempts (int): Number of probe attempts for reliability.
            
        Returns:
            dict: Probe results with success status, latency info, and quality score.
            
        Raises:
            TypeError: If config_str is not a string or attempts is not int.
            ValueError: If config_str is empty or attempts < 1.
            Exception: If V2Ray is not properly initialized or probe fails.
        """
        if not isinstance(config_str, str):
            raise TypeError("config_str must be a string")
        if not isinstance(attempts, int):
            raise TypeError("attempts must be an integer")
        if not config_str.strip():
            raise ValueError("config_str cannot be empty")
        if attempts < 1:
            raise ValueError("attempts must be at least 1")
        if not self.is_initialized:
            raise Exception("V2Ray is not properly initialized.")
        
        
        class ProbeResult(ctypes.Structure):
            _fields_ = [
                ("success", ctypes.c_int),
                ("total_ms", ctypes.c_int),
                ("dns_ms", ctypes.c_int),
                ("tcp_ms", ctypes.c_int),
                ("ttfb_ms", ctypes.c_int),
                ("score", ctypes.c_double),
                ("error_type", ctypes.c_char * 64)
            ]
        
        result_ptr = ctypes.pointer(ProbeResult())
        result = self.lib.probe_config_full(
            config_str.encode('utf-8'),
            ctypes.cast(result_ptr, ctypes.POINTER(ctypes.c_void_p)),
            self.http_port,
            self.socks_port,
            attempts
        )
        
        if result != 0:
            error_msg = self._explain_error_code(result, "Full probe failed")
            logger.error(f"Full probe failed with code {result}")
            return {
                'success': False,
                'total_ms': -1,
                'dns_ms': -1,
                'tcp_ms': -1,
                'ttfb_ms': -1,
                'score': 0.0,
                'error_type': f"Error code: {result}. {error_msg}"
            }
        
        probe_result = result_ptr.contents
        return {
            'success': bool(probe_result.success),
            'total_ms': probe_result.total_ms,
            'dns_ms': probe_result.dns_ms,
            'tcp_ms': probe_result.tcp_ms,
            'ttfb_ms': probe_result.ttfb_ms,
            'score': probe_result.score,
            'error_type': probe_result.error_type.decode('utf-8') if not probe_result.success else None
        }

    @log_function_call
    def _measure_ttfb(self, config_str, http_port=None):
        """
        [PRIVATE] Measures Time To First Byte (TTFB) for a configuration.
        
        This internal method performs an app-level end-to-end test by making a
        single HTTPS request through the proxy and measuring connection quality.
        
        Args:
            config_str (str): V2Ray configuration string (vless://, vmess://, etc.)
            http_port (int, optional): HTTP proxy port. Defaults to self.http_port.
        
        Returns:
            dict: Comprehensive test results containing:
                - platform (str): "windows" or "linux" 
                - success (bool): Whether the test succeeded
                - ttfb_ms (int or None): Time to first byte in milliseconds
                - http_status (int or None): HTTP status code of the response
                - error_message (str or None): Error description if failed
        
        Raises:
            TypeError: If config_str is not a string.
            ValueError: If config_str is empty.
            Exception: If V2Ray is not initialized or measurement fails.
        """
        if not isinstance(config_str, str):
            raise TypeError("config_str must be a string")
        if not config_str.strip():
            raise ValueError("config_str cannot be empty")
        if not self.is_initialized:
            raise Exception("V2Ray not initialized. Call set_config_string() first.")
            
        port = http_port if http_port is not None else self.http_port
        
        
        result_ptr = self.lib.measure_ttfb(config_str.encode('utf-8'), port)
        
        
        if result_ptr is None or isinstance(result_ptr, int):
            error_code = int(result_ptr) if isinstance(result_ptr, int) else -1
            error_msg = self._explain_error_code(error_code, "TTFB measurement failed")
            logger.error(f"TTFB test failed with error code {error_code}")
            
            
            return {
                'platform': 'windows' if platform.system() == 'Windows' else 'linux',
                'success': False,
                'ttfb_ms': None,
                'http_status': None,
                'error_message': f"Error code: {error_code}. {error_msg}"
            }
        
        try:
            
            import json
            json_str = ctypes.string_at(result_ptr).decode('utf-8')
            result = json.loads(json_str)
            
            
            if result['success']:
                logger.info(f"TTFB test successful: {result['ttfb_ms']}ms, status: {result['http_status']}")
            else:
                logger.error(f"TTFB test failed: {result['error_message']}")
            
            return result
        except Exception as e:
            logger.exception(f"Failed to parse TTFB result: {e}")
            
            return {
                'platform': 'windows' if platform.system() == 'Windows' else 'linux',
                'success': False,
                'ttfb_ms': None,
                'http_status': None,
                'error_message': f"Failed to parse result: {str(e)}"
            }

    def _test_single_config(self, config, timeout=10):
        """
        Test a single configuration with timeout.
        
        This method attempts multiple test methods in order of accuracy and speed:
        1. First tries TTFB for accurate real-world performance
        2. Falls back to quick probe if TTFB fails
        3. Returns failure code if all tests fail
        
        Args:
            config (str): Configuration string to test.
            timeout (int, optional): Maximum time in seconds to test. Defaults to 10.
                
        Returns:
            int: Latency in milliseconds, or -1 if failed.
            
        Raises:
            Exception: No explicit exceptions, failures are returned as -1.
        """
        try:
            
            ttfb_result = self._measure_ttfb(config)
            if ttfb_result['success']:
                return ttfb_result['ttfb_ms']
                
            
            probe_result = self._probe_quick(config)
            if probe_result['success']:
                return probe_result['total_ms']
            
            
            if 'error_message' in ttfb_result:
                logger.debug(f"Test failed: {ttfb_result['error_message']}")
            elif 'error_type' in probe_result:
                logger.debug(f"Test failed: {probe_result['error_type']}")
            else:
                logger.debug("Test failed: Unknown reason")
            
            return -1
        except Exception as e:
            logger.debug(f"Test exception: {str(e)}")
            return -1