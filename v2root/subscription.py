"""
V2Root Subscription Management

This module provides functionality to manage V2Ray subscriptions, including:
- Downloading and parsing subscription URLs
- Storing and organizing multiple subscriptions
- Auto-updating subscriptions on a schedule
- Filtering and selecting optimal configurations
- Metadata tracking and statistics
"""

import os
import re
import json
import time
import base64
import hashlib
import threading
import urllib.request
import urllib.error
from typing import List, Dict, Optional, Union, Tuple, Callable, Any
from datetime import datetime
from urllib.parse import urlparse, parse_qs, unquote

from .logger import logger, log_function_call

class SubscriptionError(Exception):
    """Base exception for subscription-related errors."""
    pass

class FetchError(SubscriptionError):
    """Exception raised when a subscription URL cannot be fetched."""
    pass

class ParseError(SubscriptionError):
    """Exception raised when subscription content cannot be parsed."""
    pass

class ConfigMetadata:
    """
    Represents metadata for a single V2Ray configuration.
    
    Stores information about protocol, server location, latency, etc.
    """
    
    def __init__(self, config_string: str):
        """
        Initialize config metadata from a configuration string.
        
        Args:
            config_string: V2Ray configuration string (vmess://, vless://, etc.)
        """
        self.config_string = config_string
        self.protocol = self._extract_protocol()
        self.name = self._extract_name()
        self.address = self._extract_address()
        self.port = self._extract_port()
        self.last_test_time = 0
        self.last_latency = -1
        self.success_count = 0
        self.failure_count = 0
        self.tags = []
    
    def __repr__(self) -> str:
        """Return a detailed string representation for debugging."""
        return (f"ConfigMetadata(name='{self.name}', protocol='{self.protocol}', "
                f"address='{self.address}:{self.port}', latency={self.last_latency}ms)")
    
    def __str__(self) -> str:
        """Return a user-friendly string representation."""
        latency_str = f"{self.last_latency}ms" if self.last_latency > 0 else "untested"
        return f"{self.name} ({self.protocol}://{self.address}:{self.port}) - {latency_str}"
    
    def _extract_protocol(self) -> str:
        """Extract protocol from config string."""
        for protocol in ['vmess', 'vless', 'trojan', 'ss', 'ssr']:
            if self.config_string.lower().startswith(f"{protocol}://"):
                return protocol
        return "unknown"
    
    def _extract_name(self) -> str:
        """Extract configuration name/remark."""
        # Try to extract from URL fragment (after #)
        if '#' in self.config_string:
            try:
                name = unquote(self.config_string.split('#', 1)[1])
                return name
            except:
                pass
        
        # Try to extract from remark parameter
        match = re.search(r'(?:remark|remarks)=([^&]+)', self.config_string)
        if match:
            try:
                name = base64.b64decode(match.group(1)).decode('utf-8')
                return name
            except:
                pass
        
        return "Unnamed Config"
    
    def _extract_address(self) -> str:
        """Extract server address."""
        try:
            # Remove protocol prefix
            temp = self.config_string.split('://', 1)[1]
            # Extract part before @ or first /
            if '@' in temp:
                temp = temp.split('@', 1)[1]
            temp = temp.split('/', 1)[0]
            temp = temp.split('?', 1)[0]
            # Extract address (before port)
            address = temp.split(':', 1)[0]
            return address
        except:
            return "unknown"
    
    def _extract_port(self) -> int:
        """Extract server port."""
        try:
            temp = self.config_string.split('://', 1)[1]
            if '@' in temp:
                temp = temp.split('@', 1)[1]
            temp = temp.split('/', 1)[0]
            temp = temp.split('?', 1)[0]
            if ':' in temp:
                port = int(temp.split(':', 1)[1])
                return port
        except:
            pass
        return 443  # Default port
    
    def update_test_result(self, latency: int, success: bool):
        """
        Update test results.
        
        Args:
            latency: Measured latency in milliseconds
            success: Whether the test was successful
        """
        self.last_test_time = time.time()
        self.last_latency = latency
        if success:
            self.success_count += 1
        else:
            self.failure_count += 1
    
    def get_success_rate(self) -> float:
        """Calculate success rate (0.0 to 1.0)."""
        total = self.success_count + self.failure_count
        if total == 0:
            return 0.0
        return self.success_count / total
    
    def to_dict(self) -> Dict:
        """Convert to dictionary for serialization."""
        return {
            'config_string': self.config_string,
            'protocol': self.protocol,
            'name': self.name,
            'address': self.address,
            'port': self.port,
            'last_test_time': self.last_test_time,
            'last_latency': self.last_latency,
            'success_count': self.success_count,
            'failure_count': self.failure_count,
            'tags': self.tags
        }
    
    @classmethod
    def from_dict(cls, data: Dict) -> 'ConfigMetadata':
        """Create from dictionary."""
        config = cls(data['config_string'])
        config.protocol = data.get('protocol', config.protocol)
        config.name = data.get('name', config.name)
        config.address = data.get('address', config.address)
        config.port = data.get('port', config.port)
        config.last_test_time = data.get('last_test_time', 0)
        config.last_latency = data.get('last_latency', -1)
        config.success_count = data.get('success_count', 0)
        config.failure_count = data.get('failure_count', 0)
        config.tags = data.get('tags', [])
        return config


class Subscription:
    """
    Represents a V2Ray subscription with enhanced features.
    
    Features:
    - Metadata tracking for each configuration
    - Automatic and manual updates
    - Filtering and sorting capabilities
    - Statistics tracking
    - Tag support
    """
    
    @log_function_call
    def __init__(self, url: str, name: Optional[str] = None, 
                 auto_update: bool = False, update_interval: int = 86400,
                 enabled: bool = True, priority: int = 0, tags: Optional[List[str]] = None):
        """
        Initialize a subscription.
        
        Args:
            url: The subscription URL
            name: A friendly name for this subscription
            auto_update: Whether to automatically update this subscription
            update_interval: Update interval in seconds (default: 24 hours)
            enabled: Whether this subscription is enabled
            priority: Priority level (higher = more important)
            tags: List of tags for categorization
            
        Raises:
            ValueError: If URL is invalid
        """
        if not url or not url.startswith(('http://', 'https://')):
            raise ValueError("Invalid subscription URL. Must start with http:// or https://")
        
        self.url = url
        self.configs: List[ConfigMetadata] = []
        self.last_update_time = 0
        self.last_fetch_success = False
        self.last_error_message = ""
        self.auto_update = auto_update
        self.update_interval = update_interval
        self.enabled = enabled
        self.priority = priority
        self.tags = tags or []
        self.update_thread = None
        self.stop_auto_update_flag = threading.Event()
        
        # Statistics
        self.total_updates = 0
        self.successful_updates = 0
        self.failed_updates = 0
        
        # Generate a unique ID for this subscription based on URL
        self.id = hashlib.md5(url.encode()).hexdigest()
        
        # Set name to domain name if not provided
        if not name:
            parsed_url = urlparse(url)
            self.name = parsed_url.netloc or "Unknown"
        else:
            self.name = name
    
    @log_function_call
    def fetch(self, timeout: int = 30) -> List[ConfigMetadata]:
        """
        Fetch and parse the subscription.
        
        Args:
            timeout: Request timeout in seconds
            
        Returns:
            List of ConfigMetadata objects
            
        Raises:
            FetchError: If subscription cannot be fetched
            ParseError: If subscription content cannot be parsed
        """
        logger.info(f"Fetching subscription: {self.name}")
        
        self.total_updates += 1
    
        request = urllib.request.Request(self.url)
        
        try:
            with urllib.request.urlopen(request, timeout=timeout) as response:
                content = response.read()
                
                # Try to decode as UTF-8 first
                try:
                    content_str = content.decode('utf-8').strip()
                except UnicodeDecodeError:
                    try:
                        content_str = content.decode('utf-8-sig').strip()
                    except UnicodeDecodeError:
                        content_str = content.decode('latin1').strip()
                
                # Update last fetch time
                self.last_update_time = time.time()
                self.last_fetch_success = True
                self.last_error_message = ""
                self.successful_updates += 1
                
                # Parse the content
                return self._parse_content(content_str)
                
        except urllib.error.URLError as e:
            self.last_fetch_success = False
            self.last_error_message = str(e)
            self.failed_updates += 1
            error_msg = f"Failed to fetch subscription {self.name}: {str(e)}"
            logger.error(error_msg)
            raise FetchError(error_msg)
        except Exception as e:
            self.last_fetch_success = False
            self.last_error_message = str(e)
            self.failed_updates += 1
            error_msg = f"Unexpected error fetching subscription {self.name}: {str(e)}"
            logger.error(error_msg)
            raise FetchError(error_msg)
    
    @log_function_call
    def _parse_content(self, content: str) -> List[ConfigMetadata]:
        """
        Parse subscription content into a list of ConfigMetadata objects.
        
        Args:
            content: Raw subscription content
            
        Returns:
            List of ConfigMetadata objects
            
        Raises:
            ParseError: If content cannot be parsed
        """
        try:
            config_strings = []
            
            # Try to decode as Base64 first
            try:
                # Check if content looks like Base64 (no spaces, special chars suggest it's encoded)
                if ' ' not in content and '\n' not in content and len(content) > 100:
                    padding = 4 - (len(content) % 4) if len(content) % 4 else 0
                    padded_content = content + "=" * padding
                    decoded = base64.b64decode(padded_content).decode('utf-8')
                    config_strings = [line.strip() for line in decoded.splitlines() if line.strip()]
                    logger.debug(f"Successfully decoded Base64 content for {self.name}")
                else:
                    # Content already has newlines, likely not Base64 encoded
                    config_strings = [line.strip() for line in content.splitlines() if line.strip()]
                    logger.debug(f"Content appears to be plain text for {self.name}")
            except Exception as e:
                logger.warning(f"Base64 decoding failed for {self.name}: {str(e)}, trying direct parsing")
                # If Base64 fails, try direct parsing
                config_strings = [line.strip() for line in content.splitlines() if line.strip()]
            
            # Log what we got before filtering
            logger.debug(f"Found {len(config_strings)} lines in subscription {self.name}")
            if config_strings:
                logger.debug(f"First line sample: {config_strings[0][:100]}...")
            
            # Validate configs (must start with supported protocol prefixes)
            valid_prefixes = ('vmess://', 'vless://', 'ss://', 'trojan://', 'ssr://')
            valid_configs = []
            
            for line in config_strings:
                # Check if line starts with any valid prefix
                is_valid = any(line.lower().startswith(p) for p in valid_prefixes)
                if is_valid:
                    valid_configs.append(line)
                elif line:  # Non-empty line that doesn't match
                    logger.debug(f"Skipping invalid line (first 50 chars): {line[:50]}...")
            
            if not valid_configs:
                # Provide more helpful error message
                error_details = f"No valid configurations found in subscription {self.name}. "
                if config_strings:
                    error_details += f"Found {len(config_strings)} lines but none started with valid prefixes: {', '.join(valid_prefixes)}. "
                    error_details += f"First line: {config_strings[0][:100]}..."
                else:
                    error_details += "Content appears to be empty after parsing."
                logger.error(error_details)
                raise ParseError(error_details)
            
            # Create ConfigMetadata objects
            new_configs = []
            for config_str in valid_configs:
                try:
                    config_meta = ConfigMetadata(config_str)
                    # Try to preserve existing metadata
                    existing = self._find_existing_config(config_str)
                    if existing:
                        config_meta.last_test_time = existing.last_test_time
                        config_meta.last_latency = existing.last_latency
                        config_meta.success_count = existing.success_count
                        config_meta.failure_count = existing.failure_count
                        config_meta.tags = existing.tags
                    new_configs.append(config_meta)
                except Exception as e:
                    logger.warning(f"Failed to parse config: {str(e)}")
            
            self.configs = new_configs
            logger.info(f"Successfully parsed {len(new_configs)} configurations from {self.name}")
            return new_configs
            
        except Exception as e:
            if not isinstance(e, ParseError):
                error_msg = f"Failed to parse subscription {self.name}: {str(e)}"
                logger.error(error_msg)
                raise ParseError(error_msg)
            else:
                raise
    
    def _find_existing_config(self, config_str: str) -> Optional[ConfigMetadata]:
        """Find existing config by string match."""
        for config in self.configs:
            if config.config_string == config_str:
                return config
        return None
    
    def update(self, timeout: int = 30) -> List[ConfigMetadata]:
        """Update the subscription (convenience method)."""
        return self.fetch(timeout)
    
    def start_auto_update(self):
        """Start auto-updating this subscription in the background."""
        if not self.auto_update:
            self.auto_update = True
            
        if self.update_thread and self.update_thread.is_alive():
            logger.info(f"Auto-update already running for subscription {self.name}")
            return
            
        self.stop_auto_update_flag.clear()
        self.update_thread = threading.Thread(
            target=self._auto_update_worker,
            daemon=True
        )
        self.update_thread.start()
        logger.info(f"Started auto-update for subscription {self.name} (interval: {self.update_interval}s)")
    
    def stop_auto_update(self):
        """Stop auto-updating this subscription."""
        if self.update_thread and self.update_thread.is_alive():
            self.stop_auto_update_flag.set()
            self.auto_update = False
            logger.info(f"Stopped auto-update for subscription {self.name}")
    
    def _auto_update_worker(self):
        """Worker thread that periodically updates the subscription."""
        while not self.stop_auto_update_flag.is_set():
            if time.time() - self.last_update_time >= self.update_interval:
                try:
                    self.fetch()
                except Exception as e:
                    logger.error(f"Auto-update failed for {self.name}: {str(e)}")
                    
            # Sleep in small intervals to allow quick shutdown
            for _ in range(min(self.update_interval, 60)):
                if self.stop_auto_update_flag.is_set():
                    break
                time.sleep(1)
    
    def filter_configs(self, 
                      protocols: Optional[List[str]] = None,
                      min_success_rate: Optional[float] = None,
                      max_latency: Optional[int] = None,
                      tags: Optional[List[str]] = None,
                      name_contains: Optional[str] = None) -> List[ConfigMetadata]:
        """
        Filter configurations based on criteria.
        
        Args:
            protocols: List of protocols to include
            min_success_rate: Minimum success rate (0.0 to 1.0)
            max_latency: Maximum latency in milliseconds
            tags: List of tags (config must have at least one)
            name_contains: Regex pattern to match in config name (case-insensitive)
            
        Returns:
            Filtered list of ConfigMetadata objects
            
        Raises:
            ValueError: If filter parameters are invalid
        """
        # Validate parameters
        if min_success_rate is not None:
            if not isinstance(min_success_rate, (int, float)):
                raise TypeError("min_success_rate must be a number")
            if not (0.0 <= min_success_rate <= 1.0):
                raise ValueError("min_success_rate must be between 0.0 and 1.0")
        
        if max_latency is not None:
            if not isinstance(max_latency, (int, float)):
                raise TypeError("max_latency must be a number")
            if max_latency < 0:
                raise ValueError("max_latency must be non-negative")
        
        if name_contains is not None:
            try:
                re.compile(name_contains, re.IGNORECASE)
            except re.error as e:
                raise ValueError(f"Invalid regex pattern in name_contains: {e}")
        
        # Check if latency/success_rate filters are being used without testing
        if (min_success_rate is not None or max_latency is not None):
            tested_count = len([c for c in self.configs if c.last_latency > 0 or (c.success_count + c.failure_count) > 0])
            if tested_count == 0:
                filter_type = 'latency' if max_latency is not None else 'success_rate'
                logger.warning(f"Filtering by {filter_type} but no configs tested. Use V2ROOT.test_connection() first.")
                print(f"\n⚠️  No configs tested yet! Can't filter by {filter_type}.")
                print(f"Test configs first: v2.test_connection(config.config_string)")
                print(f"Or batch test: v2.test_configs([config1, config2, ...])\n")
        
        filtered = self.configs.copy()
        
        # Filter by protocol
        if protocols:
            filtered = [c for c in filtered if c.protocol in protocols]
        
        # Filter by success rate
        if min_success_rate is not None:
            filtered = [c for c in filtered 
                       if (c.success_count + c.failure_count) > 0 and 
                       c.get_success_rate() >= min_success_rate]
        
        # Filter by latency
        if max_latency is not None:
            filtered = [c for c in filtered 
                       if c.last_latency > 0 and c.last_latency <= max_latency]
        
        # Filter by tags
        if tags:
            filtered = [c for c in filtered if any(tag in c.tags for tag in tags)]
        
        # Filter by name_contains using regex (case-insensitive)
        if name_contains:
            pattern = re.compile(name_contains, re.IGNORECASE)
            filtered = [c for c in filtered if pattern.search(c.name)]
        
        logger.debug(f"Filtered {len(self.configs)} configs down to {len(filtered)} configs")
        return filtered
    
    def sort_configs(self, by: str = 'latency', reverse: bool = False) -> List[ConfigMetadata]:
        """
        Sort configurations.
        
        Args:
            by: Sort criterion ('latency', 'success_rate', 'name', 'protocol')
            reverse: Whether to reverse the sort order
            
        Returns:
            Sorted list of ConfigMetadata objects
        """
        if by == 'latency':
            # Put untested configs at the end
            return sorted(self.configs, 
                         key=lambda c: (c.last_latency if c.last_latency > 0 else float('inf')),
                         reverse=reverse)
        elif by == 'success_rate':
            return sorted(self.configs, key=lambda c: c.get_success_rate(), reverse=not reverse)
        elif by == 'name':
            return sorted(self.configs, key=lambda c: c.name, reverse=reverse)
        elif by == 'protocol':
            return sorted(self.configs, key=lambda c: c.protocol, reverse=reverse)
        else:
            return self.configs
    
    def get_statistics(self) -> Dict[str, Any]:
        """Get subscription statistics."""
        return {
            'total_configs': len(self.configs),
            'protocols': self._count_by_protocol(),
            'tested_configs': len([c for c in self.configs if c.last_test_time > 0]),
            'average_latency': self._calculate_average_latency(),
            'total_updates': self.total_updates,
            'successful_updates': self.successful_updates,
            'failed_updates': self.failed_updates,
            'success_rate': self.successful_updates / self.total_updates if self.total_updates > 0 else 0
        }
    
    def _count_by_protocol(self) -> Dict[str, int]:
        """Count configurations by protocol."""
        counts = {}
        for config in self.configs:
            counts[config.protocol] = counts.get(config.protocol, 0) + 1
        return counts
    
    def _calculate_average_latency(self) -> float:
        """Calculate average latency of tested configs."""
        tested = [c for c in self.configs if c.last_latency > 0]
        if not tested:
            return 0.0
        return sum(c.last_latency for c in tested) / len(tested)
    
    def get_configs(self) -> List[str]:
        """
        Get all configuration strings from this subscription.
        
        Returns:
            List[str]: List of configuration strings
        
        Examples:
            >>> sub = manager.get_subscription(sub_id)
            >>> configs = sub.get_configs()
            >>> print(f"Found {len(configs)} configs")
            >>> print(configs[0])  # First config string
        """
        return [config.config_string for config in self.configs]
    
    def to_dict(self) -> Dict:
        """Convert subscription to a dictionary for serialization."""
        return {
            'id': self.id,
            'name': self.name,
            'url': self.url,
            'enabled': self.enabled,
            'priority': self.priority,
            'tags': self.tags,
            'auto_update': self.auto_update,
            'update_interval': self.update_interval,
            'last_update_time': self.last_update_time,
            'last_fetch_success': self.last_fetch_success,
            'last_error_message': self.last_error_message,
            'total_updates': self.total_updates,
            'successful_updates': self.successful_updates,
            'failed_updates': self.failed_updates,
            'configs': [c.to_dict() for c in self.configs]
        }
    
    @classmethod
    def from_dict(cls, data: Dict) -> 'Subscription':
        """Create a subscription from a dictionary."""
        sub = cls(
            url=data['url'],
            name=data['name'],
            auto_update=data.get('auto_update', False),
            update_interval=data.get('update_interval', 86400),
            enabled=data.get('enabled', True),
            priority=data.get('priority', 0),
            tags=data.get('tags', [])
        )
        sub.id = data['id']
        sub.last_update_time = data.get('last_update_time', 0)
        sub.last_fetch_success = data.get('last_fetch_success', False)
        sub.last_error_message = data.get('last_error_message', '')
        sub.total_updates = data.get('total_updates', 0)
        sub.successful_updates = data.get('successful_updates', 0)
        sub.failed_updates = data.get('failed_updates', 0)
        
        # Load configs with metadata
        if 'configs' in data:
            sub.configs = [ConfigMetadata.from_dict(c) for c in data['configs']]
            
        return sub


class SubscriptionManager:
    """
    Manages multiple V2Ray subscriptions with enhanced features.
    """
    
    @log_function_call
    def __init__(self, storage_dir: Optional[str] = None):
        """
        Initialize the subscription manager.
        
        Args:
            storage_dir: Directory to store subscription data
        """
        if storage_dir is None:
            import platform
            if platform.system() == "Windows":
                storage_dir = os.path.join(os.environ.get('APPDATA', ''), 'V2Root', 'subscriptions')
            else:
                storage_dir = os.path.join(os.path.expanduser('~'), '.v2root', 'subscriptions')
                
        self.storage_dir = storage_dir
        os.makedirs(storage_dir, exist_ok=True)
        
        self.subscriptions: Dict[str, Subscription] = {}
        self._load_subscriptions()
    
    @log_function_call
    def add_subscription(self, url: str, name: Optional[str] = None, 
                        auto_update: bool = False, update_interval: int = 86400,
                        enabled: bool = True, priority: int = 0, 
                        tags: Optional[List[str]] = None,
                        fetch_now: bool = True) -> Subscription:
        """Add a new subscription with enhanced options."""
        for sub in self.subscriptions.values():
            if sub.url == url:
                raise ValueError(f"Subscription URL already exists: {url}")
        
        subscription = Subscription(url, name, auto_update, update_interval, enabled, priority, tags)
        
        if fetch_now:
            try:
                subscription.fetch()
            except (FetchError, ParseError) as e:
                logger.warning(f"Failed to fetch subscription: {str(e)}")
        
        if auto_update:
            subscription.start_auto_update()
        
        self.subscriptions[subscription.id] = subscription
        self._save_subscription(subscription)
        
        logger.info(f"Added subscription: {subscription.name}")
        return subscription
    
    @log_function_call
    def remove_subscription(self, subscription_id: str) -> bool:
        """
        Remove a subscription.
        
        Args:
            subscription_id: ID of the subscription to remove
            
        Returns:
            True if subscription was removed, False if not found
        """
        if subscription_id in self.subscriptions:
            subscription = self.subscriptions[subscription_id]
            
            # Stop auto-update if running
            if subscription.auto_update:
                subscription.stop_auto_update()
            
            # Remove from dictionary
            del self.subscriptions[subscription_id]
            
            # Remove from disk
            sub_file = os.path.join(self.storage_dir, f"{subscription_id}.json")
            if os.path.exists(sub_file):
                os.remove(sub_file)
            
            logger.info(f"Removed subscription: {subscription.name}")
            return True
        else:
            logger.warning(f"Subscription not found: {subscription_id}")
            return False
    
    @log_function_call
    def get_subscription(self, subscription_id: str) -> Optional[Subscription]:
        """
        Get a subscription by ID.
        
        Args:
            subscription_id: ID of the subscription to get
            
        Returns:
            Subscription object or None if not found
        """
        return self.subscriptions.get(subscription_id)
    
    @log_function_call
    def update_subscription(self, subscription_id: str, timeout: int = 30) -> Optional[List[str]]:
        """
        Update a specific subscription.
        
        Args:
            subscription_id: ID of the subscription to update
            timeout: Request timeout in seconds
            
        Returns:
            List of V2Ray configuration strings or None if subscription not found
            
        Raises:
            FetchError: If subscription cannot be fetched
            ParseError: If subscription content cannot be parsed
        """
        subscription = self.get_subscription(subscription_id)
        if subscription:
            configs = subscription.update(timeout)
            self._save_subscription(subscription)
            return configs
        return None
    
    @log_function_call
    def update_all(self, timeout: int = 30) -> Dict[str, Union[List[str], Exception]]:
        """
        Update all subscriptions.
        
        Args:
            timeout: Request timeout in seconds
            
        Returns:
            Dictionary mapping subscription IDs to either a list of configs or an exception
        """
        results = {}
        for subscription_id, subscription in self.subscriptions.items():
            try:
                configs = subscription.update(timeout)
                results[subscription_id] = configs
                self._save_subscription(subscription)
            except Exception as e:
                results[subscription_id] = e
                logger.error(f"Failed to update subscription {subscription.name}: {str(e)}")
        
        return results
    
    @log_function_call
    def get_all_configs(self, enabled_only: bool = True) -> List[str]:
        """
        Get all configuration strings from all subscriptions.
        
        Args:
            enabled_only: If True, only return configs from enabled subscriptions (default: True)
        
        Returns:
            List[str]: List of all configuration strings
        
        Examples:
            >>> # Get all configs from all subscriptions
            >>> all_configs = manager.get_all_configs()
            >>> print(f"Total configs: {len(all_configs)}")
            
            >>> # Include disabled subscriptions too
            >>> all_configs = manager.get_all_configs(enabled_only=False)
        """
        all_configs = []
        for subscription in self.subscriptions.values():
            if not enabled_only or subscription.enabled:
                all_configs.extend(subscription.get_configs())
        return all_configs
    
    @log_function_call
    def get_configs_from_subscription(self, subscription_id: str) -> Optional[List[str]]:
        """
        Get all configuration strings from a specific subscription.
        
        Args:
            subscription_id: ID of the subscription
        
        Returns:
            Optional[List[str]]: List of configuration strings, or None if subscription not found
        
        Examples:
            >>> # Get configs from a specific subscription
            >>> configs = manager.get_configs_from_subscription(sub_id)
            >>> if configs:
            ...     print(f"Found {len(configs)} configs")
            ...     for config in configs:
            ...         print(config[:50])  # Print first 50 chars of each
        """
        subscription = self.get_subscription(subscription_id)
        if subscription:
            return subscription.get_configs()
        return None
    
    @log_function_call
    def filter_configs(self, 
                      protocols: Optional[List[str]] = None,
                      min_success_rate: Optional[float] = None,
                      max_latency: Optional[int] = None,
                      subscription_tags: Optional[List[str]] = None,
                      config_tags: Optional[List[str]] = None,
                      name_contains: Optional[str] = None) -> List[ConfigMetadata]:
        """
        Filter configurations across all subscriptions.
        
        Returns ConfigMetadata objects (not strings) for advanced filtering.
        Use get_all_configs() if you just need config strings.
        """
        # Validate parameters
        if min_success_rate is not None:
            if not isinstance(min_success_rate, (int, float)):
                raise TypeError("min_success_rate must be a number")
            if not (0.0 <= min_success_rate <= 1.0):
                raise ValueError("min_success_rate must be between 0.0 and 1.0")
        
        if max_latency is not None:
            if not isinstance(max_latency, (int, float)):
                raise TypeError("max_latency must be a number")
            if max_latency < 0:
                raise ValueError("max_latency must be non-negative")
        
        if name_contains is not None:
            try:
                re.compile(name_contains, re.IGNORECASE)
            except re.error as e:
                raise ValueError(f"Invalid regex pattern in name_contains: {e}")
        
        all_configs = self.get_all_configs()
        
        # Check if latency/success_rate filters are being used without testing
        if (min_success_rate is not None or max_latency is not None):
            tested_count = len([c for c in all_configs if c.last_latency > 0 or (c.success_count + c.failure_count) > 0])
            if tested_count == 0:
                filter_type = 'latency' if max_latency is not None else 'success_rate'
                logger.warning(f"Filtering by {filter_type} but {len(all_configs)} configs not tested. Use V2ROOT.test_connection() first.")
                print(f"\n⚠️  Can't filter by {filter_type} - No configs tested!")
                print(f"Total configs: {len(all_configs)} | Tested: 0")
                print(f"\n💡 Test configs first:")
                print(f"   v2 = V2ROOT()")
                print(f"   for config in manager.get_all_configs():")
                print(f"       latency = v2.test_connection(config.config_string)")
                print(f"       config.update_test_result(latency, success=True)")
                print(f"\n   Then filter: manager.filter_configs(max_latency=200)\n")
        
        # Filter by subscription tags
        if subscription_tags:
            filtered_configs = []
            for sub in self.subscriptions.values():
                if sub.enabled and any(tag in sub.tags for tag in subscription_tags):
                    filtered_configs.extend(sub.configs)
            all_configs = filtered_configs
        
        # Filter by protocol
        if protocols:
            all_configs = [c for c in all_configs if c.protocol in protocols]
        
        # Filter by success rate
        if min_success_rate is not None:
            all_configs = [c for c in all_configs 
                          if (c.success_count + c.failure_count) > 0 and 
                          c.get_success_rate() >= min_success_rate]
        
        # Filter by latency
        if max_latency is not None:
            all_configs = [c for c in all_configs 
                          if c.last_latency > 0 and c.last_latency <= max_latency]
        
        # Filter by config tags
        if config_tags:
            all_configs = [c for c in all_configs if any(tag in c.tags for tag in config_tags)]
        
        # Filter by name_contains using regex (case-insensitive)
        if name_contains:
            pattern = re.compile(name_contains, re.IGNORECASE)
            all_configs = [c for c in all_configs if pattern.search(c.name)]
        
        logger.debug(f"Filtered down to {len(all_configs)} configs")
        return all_configs
    
    @log_function_call
    def list_subscriptions(self) -> List[Dict]:
        """
        List all subscriptions.
        
        Returns:
            List of subscription dictionaries
        """
        return [sub.to_dict() for sub in self.subscriptions.values()]
    
    @log_function_call
    def _load_subscriptions(self):
        """Load subscriptions from storage directory."""
        try:
            # Find all JSON files in storage directory
            for filename in os.listdir(self.storage_dir):
                if filename.endswith('.json'):
                    file_path = os.path.join(self.storage_dir, filename)
                    try:
                        with open(file_path, 'r', encoding='utf-8') as f:
                            data = json.load(f)
                            subscription = Subscription.from_dict(data)
                            self.subscriptions[subscription.id] = subscription
                            
                            # Start auto-update if enabled
                            if subscription.auto_update:
                                subscription.start_auto_update()
                    except Exception as e:
                        logger.error(f"Failed to load subscription from {file_path}: {str(e)}")
            
            logger.info(f"Loaded {len(self.subscriptions)} subscriptions")
        except Exception as e:
            logger.error(f"Failed to load subscriptions: {str(e)}")
    
    @log_function_call
    def _save_subscription(self, subscription: Subscription):
        """Save a single subscription to storage directory."""
        try:
            file_path = os.path.join(self.storage_dir, f"{subscription.id}.json")
            data = subscription.to_dict()
            
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False, indent=2)
        except Exception as e:
            logger.error(f"Failed to save subscription {subscription.name}: {str(e)}")
