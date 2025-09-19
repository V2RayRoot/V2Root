#!/usr/bin/env python3
"""
V2Root Subscription Manager Example

This example demonstrates how to:
1. Add and manage V2Ray subscriptions
2. Update subscriptions and retrieve configurations
3. Filter configurations by criteria
4. Test configurations from subscriptions
5. Apply the best performing configuration
"""

import os
import sys
import time
from datetime import datetime

# Add the parent directory to sys.path to import v2root
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from v2root import V2ROOT, SubscriptionManager, configure_logger, info, error
from colorama import init, Fore, Style

init(autoreset=True)

def format_timestamp(timestamp):
    """Format a timestamp as a readable date/time."""
    if timestamp == 0:
        return "Never"
    return datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S")

def main():
    # Initialize logging with debug level
    configure_logger(log_level=20)  # INFO level
    
    print(f"{Fore.CYAN}V2Root Subscription Manager Example{Style.RESET_ALL}")
    print(f"{Fore.CYAN}===================================={Style.RESET_ALL}")
    
    # Initialize the subscription manager
    sub_manager = SubscriptionManager()
    
    # Initialize V2ROOT with custom ports
    try:
        v2root = V2ROOT(http_port=2300, socks_port=2301)
    except Exception as e:
        print(f"{Fore.RED}Error initializing V2ROOT: {str(e)}{Style.RESET_ALL}")
        print(f"{Fore.YELLOW}You can still manage subscriptions, but testing will not be available.{Style.RESET_ALL}")
        v2root = None
    
    while True:
        print("\nOptions:")
        print("1. Add a subscription")
        print("2. List subscriptions")
        print("3. Update a subscription")
        print("4. Update all subscriptions")
        print("5. View configurations from a subscription")
        print("6. Filter configurations")
        print("7. Test configurations and find the best")
        print("8. Remove a subscription")
        print("9. Exit")
        
        choice = input("\nEnter your choice (1-9): ")
        
        if choice == '1':
            # Add a subscription
            url = input("Enter the subscription URL: ")
            name = input("Enter a name for this subscription (or press Enter for auto-name): ")
            auto_update_input = input("Enable auto-update? (y/n): ")
            auto_update = auto_update_input.lower() == 'y'
            
            try:
                name = name if name.strip() else None
                sub = sub_manager.add_subscription(url, name, auto_update)
                print(f"{Fore.GREEN}Successfully added subscription: {sub.name}{Style.RESET_ALL}")
                print(f"{Fore.GREEN}Found {len(sub.configs)} configurations{Style.RESET_ALL}")
            except Exception as e:
                print(f"{Fore.RED}Error adding subscription: {str(e)}{Style.RESET_ALL}")
        
        elif choice == '2':
            # List subscriptions
            subscriptions = sub_manager.list_subscriptions()
            if not subscriptions:
                print(f"{Fore.YELLOW}No subscriptions found{Style.RESET_ALL}")
                continue
            
            print(f"{Fore.CYAN}Found {len(subscriptions)} subscriptions:{Style.RESET_ALL}")
            for i, sub in enumerate(subscriptions, 1):
                last_update = format_timestamp(sub['last_update_time'])
                auto_update = "Enabled" if sub['auto_update'] else "Disabled"
                print(f"{i}. {Fore.GREEN}{sub['name']}{Style.RESET_ALL} ({sub['url']})")
                print(f"   ID: {sub['id']}")
                print(f"   Configs: {sub['config_count']}")
                print(f"   Last Updated: {last_update}")
                print(f"   Auto-update: {auto_update}")
                print()
        
        elif choice == '3':
            # Update a subscription
            subscriptions = sub_manager.list_subscriptions()
            if not subscriptions:
                print(f"{Fore.YELLOW}No subscriptions found{Style.RESET_ALL}")
                continue
            
            for i, sub in enumerate(subscriptions, 1):
                print(f"{i}. {sub['name']}")
            
            index = input("Enter the number of the subscription to update: ")
            try:
                index = int(index) - 1
                if 0 <= index < len(subscriptions):
                    sub_id = subscriptions[index]['id']
                    print(f"{Fore.CYAN}Updating subscription...{Style.RESET_ALL}")
                    try:
                        configs = sub_manager.update_subscription(sub_id)
                        print(f"{Fore.GREEN}Successfully updated subscription{Style.RESET_ALL}")
                        print(f"{Fore.GREEN}Found {len(configs)} configurations{Style.RESET_ALL}")
                    except Exception as e:
                        print(f"{Fore.RED}Error updating subscription: {str(e)}{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}Invalid selection{Style.RESET_ALL}")
            except ValueError:
                print(f"{Fore.RED}Please enter a valid number{Style.RESET_ALL}")
        
        elif choice == '4':
            # Update all subscriptions
            print(f"{Fore.CYAN}Updating all subscriptions...{Style.RESET_ALL}")
            results = sub_manager.update_all()
            
            success = 0
            failed = 0
            for sub_id, result in results.items():
                sub = sub_manager.get_subscription(sub_id)
                if isinstance(result, list):
                    print(f"{Fore.GREEN}Updated {sub.name}: {len(result)} configurations{Style.RESET_ALL}")
                    success += 1
                else:
                    print(f"{Fore.RED}Failed to update {sub.name}: {str(result)}{Style.RESET_ALL}")
                    failed += 1
            
            print(f"{Fore.CYAN}Update complete: {success} succeeded, {failed} failed{Style.RESET_ALL}")
        
        elif choice == '5':
            # View configurations from a subscription
            subscriptions = sub_manager.list_subscriptions()
            if not subscriptions:
                print(f"{Fore.YELLOW}No subscriptions found{Style.RESET_ALL}")
                continue
            
            for i, sub in enumerate(subscriptions, 1):
                print(f"{i}. {sub['name']} ({sub['config_count']} configs)")
            
            index = input("Enter the number of the subscription to view: ")
            try:
                index = int(index) - 1
                if 0 <= index < len(subscriptions):
                    sub_id = subscriptions[index]['id']
                    sub = sub_manager.get_subscription(sub_id)
                    
                    if not sub.configs:
                        print(f"{Fore.YELLOW}No configurations found in this subscription{Style.RESET_ALL}")
                        continue
                    
                    print(f"{Fore.CYAN}Configurations in {sub.name}:{Style.RESET_ALL}")
                    max_configs_to_show = min(10, len(sub.configs))
                    for i, config in enumerate(sub.configs[:max_configs_to_show], 1):
                        # Extract and show a readable name if possible
                        name = "Unknown"
                        if '#' in config:
                            name = config.split('#', 1)[1]
                        print(f"{i}. {name}")
                    
                    if len(sub.configs) > max_configs_to_show:
                        print(f"...and {len(sub.configs) - max_configs_to_show} more")
                else:
                    print(f"{Fore.RED}Invalid selection{Style.RESET_ALL}")
            except ValueError:
                print(f"{Fore.RED}Please enter a valid number{Style.RESET_ALL}")
        
        elif choice == '6':
            # Filter configurations
            print(f"{Fore.CYAN}Filter configurations{Style.RESET_ALL}")
            
            # Protocol filter
            print("Select protocols to include (leave blank to include all):")
            print("1. VLESS")
            print("2. VMess")
            print("3. Shadowsocks")
            print("4. Trojan")
            
            protocol_input = input("Enter numbers separated by commas (e.g., 1,2): ")
            protocols = []
            if protocol_input.strip():
                protocol_map = {
                    '1': 'vless',
                    '2': 'vmess',
                    '3': 'ss',
                    '4': 'trojan'
                }
                for num in protocol_input.split(','):
                    num = num.strip()
                    if num in protocol_map:
                        protocols.append(protocol_map[num])
            
            # Country filter
            country_input = input("Enter country codes to include (e.g., US,JP) or leave blank: ")
            countries = [c.strip().upper() for c in country_input.split(',')] if country_input.strip() else None
            
            # Name filter
            name_contains = input("Enter text to search in names or leave blank: ")
            name_contains = name_contains.strip() if name_contains.strip() else None
            
            # Apply filters
            configs = sub_manager.filter_configs(protocols, countries, name_contains)
            
            print(f"{Fore.GREEN}Found {len(configs)} configurations matching filters{Style.RESET_ALL}")
            
            if configs:
                max_configs_to_show = min(10, len(configs))
                for i, config in enumerate(configs[:max_configs_to_show], 1):
                    # Extract and show a readable name if possible
                    name = "Unknown"
                    if '#' in config:
                        name = config.split('#', 1)[1]
                    print(f"{i}. {name}")
                
                if len(configs) > max_configs_to_show:
                    print(f"...and {len(configs) - max_configs_to_show} more")
                
                # Option to test these configs
                if v2root:
                    test_input = input("Do you want to test these configurations? (y/n): ")
                    if test_input.lower() == 'y':
                        # Redirect to the testing option
                        print(f"{Fore.CYAN}Testing filtered configurations...{Style.RESET_ALL}")
                        test_configs(v2root, configs)
        
        elif choice == '7':
            # Test configurations and find the best
            if not v2root:
                print(f"{Fore.RED}V2ROOT is not initialized. Cannot test configurations.{Style.RESET_ALL}")
                continue
            
            # Get configs from all subscriptions
            configs = sub_manager.get_all_configs()
            
            if not configs:
                print(f"{Fore.YELLOW}No configurations found in subscriptions{Style.RESET_ALL}")
                continue
            
            print(f"{Fore.CYAN}Found {len(configs)} total configurations{Style.RESET_ALL}")
            
            # Option to filter before testing
            filter_input = input("Do you want to filter configurations before testing? (y/n): ")
            if filter_input.lower() == 'y':
                # Protocol filter
                print("Select protocols to include (leave blank to include all):")
                print("1. VLESS")
                print("2. VMess")
                print("3. Shadowsocks")
                print("4. Trojan")
                
                protocol_input = input("Enter numbers separated by commas (e.g., 1,2): ")
                protocols = []
                if protocol_input.strip():
                    protocol_map = {
                        '1': 'vless',
                        '2': 'vmess',
                        '3': 'ss',
                        '4': 'trojan'
                    }
                    for num in protocol_input.split(','):
                        num = num.strip()
                        if num in protocol_map:
                            protocols.append(protocol_map[num])
                
                # Country filter
                country_input = input("Enter country codes to include (e.g., US,JP) or leave blank: ")
                countries = [c.strip().upper() for c in country_input.split(',')] if country_input.strip() else None
                
                # Name filter
                name_contains = input("Enter text to search in names or leave blank: ")
                name_contains = name_contains.strip() if name_contains.strip() else None
                
                # Apply filters
                configs = sub_manager.filter_configs(protocols, countries, name_contains)
                print(f"{Fore.GREEN}Found {len(configs)} configurations matching filters{Style.RESET_ALL}")
            
            if not configs:
                print(f"{Fore.YELLOW}No configurations to test after filtering{Style.RESET_ALL}")
                continue
            
            # Test the configs
            test_configs(v2root, configs)
        
        elif choice == '8':
            # Remove a subscription
            subscriptions = sub_manager.list_subscriptions()
            if not subscriptions:
                print(f"{Fore.YELLOW}No subscriptions found{Style.RESET_ALL}")
                continue
            
            for i, sub in enumerate(subscriptions, 1):
                print(f"{i}. {sub['name']} ({sub['url']})")
            
            index = input("Enter the number of the subscription to remove: ")
            try:
                index = int(index) - 1
                if 0 <= index < len(subscriptions):
                    sub_id = subscriptions[index]['id']
                    confirm = input(f"Are you sure you want to remove '{subscriptions[index]['name']}'? (y/n): ")
                    if confirm.lower() == 'y':
                        if sub_manager.remove_subscription(sub_id):
                            print(f"{Fore.GREEN}Subscription removed successfully{Style.RESET_ALL}")
                        else:
                            print(f"{Fore.RED}Failed to remove subscription{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}Invalid selection{Style.RESET_ALL}")
            except ValueError:
                print(f"{Fore.RED}Please enter a valid number{Style.RESET_ALL}")
        
        elif choice == '9':
            # Exit
            print(f"{Fore.CYAN}Exiting...{Style.RESET_ALL}")
            # Clean up
            if v2root:
                try:
                    v2root.reset_network_proxy()
                except:
                    pass
            break
        
        else:
            print(f"{Fore.RED}Invalid choice. Please enter a number between 1 and 9.{Style.RESET_ALL}")

def test_configs(v2root, configs):
    """Test configurations and find the best performing one."""
    print(f"{Fore.CYAN}Testing configurations...{Style.RESET_ALL}")
    
    # Ask for test parameters
    max_configs_input = input("Maximum number of configs to test (default: 20): ")
    max_configs = int(max_configs_input) if max_configs_input.strip() and max_configs_input.isdigit() else 20
    
    timeout_input = input("Timeout in seconds for each test (default: 5): ")
    timeout = int(timeout_input) if timeout_input.strip() and timeout_input.isdigit() else 5
    
    parallel_input = input("Test in parallel? Slower machines should use 'n' (y/n): ")
    parallel = parallel_input.lower() == 'y'
    
    # Limit to max_configs
    if len(configs) > max_configs:
        print(f"{Fore.YELLOW}Limiting to {max_configs} configurations{Style.RESET_ALL}")
        configs = configs[:max_configs]
    
    # Test the configurations
    start_time = time.time()
    results = v2root.batch_test(configs, timeout, parallel)
    elapsed = time.time() - start_time
    
    if not results:
        print(f"{Fore.RED}No working configurations found{Style.RESET_ALL}")
        return
    
    print(f"{Fore.GREEN}Testing completed in {elapsed:.2f} seconds{Style.RESET_ALL}")
    print(f"{Fore.GREEN}Found {len(results)} working configurations{Style.RESET_ALL}")
    
    # Show top configurations
    print(f"{Fore.CYAN}Top configurations by latency:{Style.RESET_ALL}")
    max_to_show = min(5, len(results))
    for i, (config, latency) in enumerate(results[:max_to_show], 1):
        # Extract and show a readable name if possible
        name = "Unknown"
        if '#' in config:
            name = config.split('#', 1)[1]
        print(f"{i}. {Fore.GREEN}{name}{Style.RESET_ALL} - Latency: {latency}ms")
    
    # Option to use the best configuration
    use_best = input("Do you want to use the best configuration? (y/n): ")
    if use_best.lower() == 'y':
        best_config = results[0][0]
        try:
            v2root.set_config_string(best_config)
            v2root.start()
            print(f"{Fore.GREEN}Successfully applied the best configuration{Style.RESET_ALL}")
        except Exception as e:
            print(f"{Fore.RED}Error applying configuration: {str(e)}{Style.RESET_ALL}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"{Fore.CYAN}\nExiting...{Style.RESET_ALL}")
        try:
            # Clean up
            v2root = V2ROOT()
            v2root.reset_network_proxy()
        except:
            pass
        sys.exit(0)
