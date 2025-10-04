#!/usr/bin/env python3
"""
V2Root Logging System Example

This example demonstrates how to:
1. Configure the logging system
2. Use different log levels
3. Get log files and enable/disable file logging
4. Create module-specific loggers
"""

import os
import sys
import time

# Add the parent directory to sys.path to import v2root
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from v2root import configure_logger, get_logger, debug, info, warning, error, critical
from v2root import V2ROOT

def demonstrate_log_levels():
    """Demonstrate different log levels."""
    print("\n=== Log Levels ===")
    debug("This is a DEBUG message - detailed information for debugging")
    info("This is an INFO message - confirmation that things are working")
    warning("This is a WARNING message - something unexpected happened")
    error("This is an ERROR message - something failed but program continues")
    critical("This is a CRITICAL message - serious error, program may be unable to continue")
    
    # Create a module-specific logger
    module_logger = get_logger("demo")
    module_logger.info("This message comes from the 'demo' logger")

def demonstrate_logger_configuration():
    """Demonstrate how to configure the logger."""
    print("\n=== Logger Configuration ===")
    
    # Change log level
    print("Changing log level to DEBUG...")
    configure_logger(log_level=10)  # DEBUG = 10
    debug("This DEBUG message should now be visible")
    
    # Change log level back to INFO
    print("Changing log level back to INFO...")
    configure_logger(log_level=20)  # INFO = 20
    debug("This DEBUG message should NOT be visible")
    info("This INFO message should be visible")
    
    # Disable file logging
    print("Disabling file logging...")
    configure_logger(log_to_file=False)
    info("This message will only go to console, not to file")
    
    # Re-enable file logging with custom directory
    print("Enabling file logging with custom directory...")
    log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
    configure_logger(log_to_file=True, log_dir=log_dir)
    info(f"This message will go to console and file in {log_dir}")
    
    # Configure maximum file size and backup count
    print("Configuring log rotation...")
    configure_logger(max_file_size=1024*1024, backup_count=5)  # 1MB, 5 backups
    info("Log files will now rotate at 1MB with 5 backup files")

def demonstrate_v2root_integration():
    """Demonstrate how logging is integrated with V2ROOT."""
    print("\n=== V2ROOT Integration ===")
    
    try:
        # Set DEBUG level to see initialization logs
        configure_logger(log_level=10)
        
        # Initialize V2ROOT with custom ports
        v2root = V2ROOT(http_port=2300, socks_port=2301)
        
        # Test a method that will be logged
        try:
            # This will likely fail but shows how errors are logged
            v2root.ping_server("example.com", 80)
        except Exception as e:
            print(f"Expected error: {str(e)}")
            
    except Exception as e:
        print(f"Error initializing V2ROOT: {str(e)}")
        
def main():
    """Main function demonstrating the logging system."""
    print("V2Root Logging System Example")
    print("=============================")
    
    # Start with default logging configuration
    info("Starting logging demonstration")
    
    # Show log levels
    demonstrate_log_levels()
    
    # Show configuration options
    demonstrate_logger_configuration()
    
    # Show V2ROOT integration
    demonstrate_v2root_integration()
    
    print("\nLogging demonstration completed")
    info("Logging demonstration completed")

if __name__ == "__main__":
    main()
