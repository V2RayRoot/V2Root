#!/usr/bin/env python3
"""
V2Root Configuration Manager Example

This example demonstrates how to:
1. Save and load V2Ray configurations
2. Test multiple configurations
3. Select the best performing configuration
4. Apply and start a configuration
"""

import os
import sys
import time
from colorama import init, Fore, Style

# Add the parent directory to sys.path to import v2root
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from v2root import V2ROOT

init(autoreset=True)

def main():
    # Initialize V2ROOT with custom ports
    v2root = V2ROOT(http_port=2300, socks_port=2301)
    
    print(f"{Fore.CYAN}V2Root Configuration Manager Example{Style.RESET_ALL}")
    print(f"{Fore.CYAN}===================================={Style.RESET_ALL}")
    
    while True:
        print("\nOptions:")
        print("1. Save a new configuration")
        print("2. List saved configurations")
        print("3. Load and test a configuration")
        print("4. Batch test multiple configurations")
        print("5. Reset network proxy")
        print("6. Exit")
        
        choice = input("\nEnter your choice (1-6): ")
        
        if choice == '1':
            # Save a new configuration
            config_str = input("Enter the V2Ray configuration string: ")
            name = input("Enter a name for this configuration (or press Enter for auto-name): ")
            name = name if name.strip() else None
            
            try:
                file_path = v2root.save_config(config_str, name)
                print(f"{Fore.GREEN}Configuration saved to {file_path}{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.RED}Error: {str(e)}{Style.RESET_ALL}")
                
        elif choice == '2':
            # List saved configurations
            configs = v2root.list_saved_configs()
            
        elif choice == '3':
            # Load and test a configuration
            configs = v2root.list_saved_configs()
            if not configs:
                continue
                
            index = input("Enter the number of the configuration to load: ")
            try:
                index = int(index) - 1
                if 0 <= index < len(configs):
                    config = v2root.load_saved_config(configs[index])
                    
                    print(f"{Fore.CYAN}Testing configuration...{Style.RESET_ALL}")
                    try:
                        latency = v2root.test_connection(config)
                        print(f"{Fore.GREEN}Test successful! Latency: {latency}ms{Style.RESET_ALL}")
                        
                        start = input("Do you want to start V2Ray with this configuration? (y/n): ")
                        if start.lower() == 'y':
                            v2root.set_config_string(config)
                            v2root.start()
                    except Exception as e:
                        print(f"{Fore.RED}Test failed: {str(e)}{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}Invalid selection{Style.RESET_ALL}")
            except ValueError:
                print(f"{Fore.RED}Please enter a valid number{Style.RESET_ALL}")
                
        elif choice == '4':
            # Batch test multiple configurations
            configs = v2root.list_saved_configs()
            if not configs:
                continue
                
            config_list = []
            for config_name in configs:
                try:
                    config = v2root.load_saved_config(config_name)
                    config_list.append(config)
                except Exception as e:
                    print(f"{Fore.RED}Error loading {config_name}: {str(e)}{Style.RESET_ALL}")
                    
            if not config_list:
                print(f"{Fore.RED}No valid configurations to test{Style.RESET_ALL}")
                continue
                
            print(f"{Fore.CYAN}Testing {len(config_list)} configurations...{Style.RESET_ALL}")
            timeout = input("Enter timeout in seconds (default: 10): ")
            timeout = int(timeout) if timeout.strip() and timeout.isdigit() else 10
            
            parallel = input("Test in parallel? (slower machines should use 'n') (y/n): ")
            parallel = parallel.lower() == 'y'
            
            start_time = time.time()
            results = v2root.batch_test(config_list, timeout, parallel)
            elapsed = time.time() - start_time
            
            print(f"{Fore.CYAN}Testing completed in {elapsed:.2f} seconds{Style.RESET_ALL}")
            
            if results:
                print(f"{Fore.GREEN}Top 3 configurations by latency:{Style.RESET_ALL}")
                for i, (config, latency) in enumerate(results[:3], 1):
                    print(f"{i}. Latency: {latency}ms - Config: {config[:50]}...")
                    
                use_best = input("Do you want to use the best configuration? (y/n): ")
                if use_best.lower() == 'y':
                    best_config = results[0][0]
                    v2root.set_config_string(best_config)
                    v2root.start()
            else:
                print(f"{Fore.RED}No working configurations found{Style.RESET_ALL}")
                
        elif choice == '5':
            # Reset network proxy
            try:
                v2root.reset_network_proxy()
            except Exception as e:
                print(f"{Fore.RED}Error: {str(e)}{Style.RESET_ALL}")
                
        elif choice == '6':
            # Exit
            try:
                v2root.reset_network_proxy()
            except:
                pass
            print(f"{Fore.CYAN}Exiting...{Style.RESET_ALL}")
            break
            
        else:
            print(f"{Fore.RED}Invalid choice. Please enter a number between 1 and 6.{Style.RESET_ALL}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"{Fore.CYAN}\nExiting...{Style.RESET_ALL}")
        try:
            v2root = V2ROOT()
            v2root.reset_network_proxy()
        except:
            pass
        sys.exit(0)
