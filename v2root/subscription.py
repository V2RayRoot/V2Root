"""
V2Root Subscription Management

This module provides functionality to manage V2Ray subscriptions, including:
- Downloading and parsing subscription URLs
- Storing and organizing multiple subscriptions
- Auto-updating subscriptions on a schedule
- Filtering and selecting optimal configurations
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
from typing import List, Dict, Optional, Union, Tuple, Callable
from datetime import datetime
from urllib.parse import urlparse

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

class Subscription:
    """
    Represents a V2Ray subscription.
    
    A subscription is a URL that returns a Base64-encoded list of V2Ray configurations.
    This class handles fetching, parsing, and managing these configurations.
    """
    
    @log_function_call
    def __init__(self, url: str, name: Optional[str] = None, 
                 auto_update: bool = False, update_interval: int = 86400):
        """
        Initialize a subscription.
        
        Args:
            url: The subscription URL
            name: A friendly name for this subscription (defaults to URL domain)
            auto_update: Whether to automatically update this subscription
            update_interval: Update interval in seconds (default: 24 hours)
            
        Raises:
            ValueError: If URL is invalid
        """
        if not url or not url.startswith(('http://', 'https://')):
            raise ValueError("Invalid subscription URL. Must start with http:// or https://")
        
        self.url = url
        self.configs = []
        self.last_update_time = 0
        self.auto_update = auto_update
        self.update_interval = update_interval
        self.update_thread = None
        self.stop_auto_update = threading.Event()
        
        # Generate a unique ID for this subscription based on URL
        self.id = hashlib.md5(url.encode()).hexdigest()
        
        # Set name to domain name if not provided
        if not name:
            parsed_url = urlparse(url)
            self.name = parsed_url.netloc
        else:
            self.name = name
    
    @log_function_call
    def fetch(self, timeout: int = 30) -> List[str]:
        """
        Fetch and parse the subscription.
        
        Args:
            timeout: Request timeout in seconds
            
        Returns:
            List of V2Ray configuration strings
            
        Raises:
            FetchError: If subscription cannot be fetched
            ParseError: If subscription content cannot be parsed
        """
        logger.info(f"Fetching subscription: {self.name}")
        
        # Create request with appropriate headers
        headers = {
            'User-Agent': 'V2Root/1.1.2',
            'Accept': 'text/plain, text/html',
            'Accept-Encoding': 'gzip, deflate',
            'Connection': 'close'
        }
        
        request = urllib.request.Request(self.url, headers=headers)
        
        try:
            with urllib.request.urlopen(request, timeout=timeout) as response:
                content = response.read()
                
                # Try to decode as UTF-8 first
                try:
                    content_str = content.decode('utf-8').strip()
                except UnicodeDecodeError:
                    # If that fails, try other encodings
                    try:
                        content_str = content.decode('utf-8-sig').strip()
                    except UnicodeDecodeError:
                        content_str = content.decode('latin1').strip()
                
                # Update last fetch time
                self.last_update_time = time.time()
                
                # Parse the content
                return self._parse_content(content_str)
                
        except urllib.error.URLError as e:
            error_msg = f"Failed to fetch subscription {self.name}: {str(e)}"
            logger.error(error_msg)
            raise FetchError(error_msg)
        except Exception as e:
            error_msg = f"Unexpected error fetching subscription {self.name}: {str(e)}"
            logger.error(error_msg)
            raise FetchError(error_msg)
    
    @log_function_call
    def _parse_content(self, content: str) -> List[str]:
        """
        Parse subscription content into a list of V2Ray configurations.
        
        Args:
            content: Raw subscription content
            
        Returns:
            List of V2Ray configuration strings
            
        Raises:
            ParseError: If content cannot be parsed
        """
        try:
            # Try to decode as Base64
            try:
                # Add padding if needed
                padding = 4 - (len(content) % 4) if len(content) % 4 else 0
                content = content + "=" * padding
                
                # Decode Base64
                decoded = base64.b64decode(content).decode('utf-8')
                
                # Split into lines and filter empty lines
                configs = [line.strip() for line in decoded.splitlines() if line.strip()]
                
            except Exception as e:
                # If Base64 decoding fails, try direct parsing (some providers don't encode)
                logger.warning(f"Base64 decoding failed for {self.name}, trying direct parsing")
                configs = [line.strip() for line in content.splitlines() if line.strip()]
            
            # Validate configs (must start with supported protocol prefixes)
            valid_prefixes = ('vmess://', 'vless://', 'ss://', 'trojan://')
            valid_configs = [c for c in configs if any(c.startswith(p) for p in valid_prefixes)]
            
            if not valid_configs:
                raise ParseError(f"No valid configurations found in subscription {self.name}")
            
            # Store valid configs and return them
            self.configs = valid_configs
            logger.info(f"Successfully parsed {len(valid_configs)} configurations from {self.name}")
            return valid_configs
            
        except Exception as e:
            if not isinstance(e, ParseError):
                error_msg = f"Failed to parse subscription {self.name}: {str(e)}"
                logger.error(error_msg)
                raise ParseError(error_msg)
            else:
                raise
    
    def update(self, timeout: int = 30) -> List[str]:
        """
        Update the subscription.
        
        This is a convenience method that calls fetch().
        
        Args:
            timeout: Request timeout in seconds
            
        Returns:
            List of V2Ray configuration strings
        """
        return self.fetch(timeout)
    
    def start_auto_update(self):
        """
        Start auto-updating this subscription in the background.
        
        Only one update thread will be started per subscription.
        """
        if not self.auto_update:
            self.auto_update = True
            
        if self.update_thread and self.update_thread.is_alive():
            logger.info(f"Auto-update already running for subscription {self.name}")
            return
            
        self.stop_auto_update.clear()
        self.update_thread = threading.Thread(
            target=self._auto_update_worker,
            daemon=True
        )
        self.update_thread.start()
        logger.info(f"Started auto-update for subscription {self.name} (interval: {self.update_interval}s)")
    
    def stop_auto_update(self):
        """Stop auto-updating this subscription."""
        if self.update_thread and self.update_thread.is_alive():
            self.stop_auto_update.set()
            self.auto_update = False
            logger.info(f"Stopped auto-update for subscription {self.name}")
    
    def _auto_update_worker(self):
        """Worker thread that periodically updates the subscription."""
        while not self.stop_auto_update.is_set():
            # Check if it's time to update
            if time.time() - self.last_update_time >= self.update_interval:
                try:
                    self.fetch()
                except Exception as e:
                    logger.error(f"Auto-update failed for {self.name}: {str(e)}")
                    
            # Sleep for a while, checking periodically if we should stop
            for _ in range(min(self.update_interval, 60)):
                if self.stop_auto_update.is_set():
                    break
                time.sleep(1)
    
    def to_dict(self) -> Dict:
        """Convert subscription to a dictionary for serialization."""
        return {
            'id': self.id,
            'name': self.name,
            'url': self.url,
            'auto_update': self.auto_update,
            'update_interval': self.update_interval,
            'last_update_time': self.last_update_time,
            'config_count': len(self.configs)
        }
    
    @classmethod
    def from_dict(cls, data: Dict) -> 'Subscription':
        """Create a subscription from a dictionary."""
        sub = cls(
            url=data['url'],
            name=data['name'],
            auto_update=data.get('auto_update', False),
            update_interval=data.get('update_interval', 86400)
        )
        sub.id = data['id']
        sub.last_update_time = data.get('last_update_time', 0)
        
        # If we have configs stored, load them
        if 'configs' in data:
            sub.configs = data['configs']
            
        return sub


class SubscriptionManager:
    """
    Manages multiple V2Ray subscriptions.
    
    This class provides functionality to add, remove, update, and select
    configurations from multiple subscriptions.
    """
    
    @log_function_call
    def __init__(self, storage_dir: Optional[str] = None):
        """
        Initialize the subscription manager.
        
        Args:
            storage_dir: Directory to store subscription data
                         (default: platform-specific user data directory)
        """
        # Determine storage directory
        if storage_dir is None:
            import platform
            if platform.system() == "Windows":
                storage_dir = os.path.join(os.environ.get('APPDATA', ''), 'V2Root', 'subscriptions')
            else:
                storage_dir = os.path.join(os.path.expanduser('~'), '.v2root', 'subscriptions')
                
        self.storage_dir = storage_dir
        os.makedirs(storage_dir, exist_ok=True)
        
        # Dictionary to store subscriptions by ID
        self.subscriptions: Dict[str, Subscription] = {}
        
        # Load existing subscriptions
        self._load_subscriptions()
    
    @log_function_call
    def add_subscription(self, url: str, name: Optional[str] = None, 
                        auto_update: bool = False, update_interval: int = 86400,
                        fetch_now: bool = True) -> Subscription:
        """
        Add a new subscription.
        
        Args:
            url: The subscription URL
            name: A friendly name for this subscription
            auto_update: Whether to automatically update this subscription
            update_interval: Update interval in seconds
            fetch_now: Whether to fetch the subscription immediately
            
        Returns:
            The added Subscription object
            
        Raises:
            ValueError: If URL is invalid or already exists
        """
        # Check if URL already exists
        for sub in self.subscriptions.values():
            if sub.url == url:
                raise ValueError(f"Subscription URL already exists: {url}")
        
        # Create the subscription
        subscription = Subscription(url, name, auto_update, update_interval)
        
        # Fetch the subscription if requested
        if fetch_now:
            try:
                subscription.fetch()
            except (FetchError, ParseError) as e:
                logger.warning(f"Failed to fetch subscription: {str(e)}")
                # Continue anyway, as we'll store the subscription even if initial fetch fails
        
        # Start auto-update if enabled
        if auto_update:
            subscription.start_auto_update()
        
        # Store the subscription
        self.subscriptions[subscription.id] = subscription
        
        # Save to disk
        self._save_subscriptions()
        
        logger.info(f"Added subscription: {subscription.name} ({url})")
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
    def get_all_configs(self) -> List[str]:
        """
        Get all configurations from all subscriptions.
        
        Returns:
            List of all V2Ray configuration strings
        """
        all_configs = []
        for subscription in self.subscriptions.values():
            all_configs.extend(subscription.configs)
        return all_configs
    
    @log_function_call
    def filter_configs(self, 
                      protocols: Optional[List[str]] = None, 
                      countries: Optional[List[str]] = None,
                      name_contains: Optional[str] = None) -> List[str]:
        """
        Filter configurations based on criteria.
        
        Args:
            protocols: List of protocols to include (e.g., ['vmess', 'vless'])
            countries: List of country codes to include (e.g., ['US', 'JP'])
            name_contains: String that must be present in config name
            
        Returns:
            Filtered list of V2Ray configuration strings
        """
        configs = self.get_all_configs()
        
        # Filter by protocol
        if protocols:
            filtered = []
            for config in configs:
                for protocol in protocols:
                    if config.lower().startswith(f"{protocol.lower()}://"):
                        filtered.append(config)
                        break
            configs = filtered
        
        # Filter by country code or name_contains
        # This is tricky since the country info is encoded in the config name
        # which varies by provider. We'll do a basic text search.
        if countries or name_contains:
            filtered = []
            for config in configs:
                # Try to extract the config name
                match = re.search(r'(?:remark|remarks)=([^&]+)', config)
                if match:
                    try:
                        name = base64.b64decode(match.group(1)).decode('utf-8')
                    except:
                        name = match.group(1)
                else:
                    # For some formats, the name might be after a '#'
                    parts = config.split('#', 1)
                    name = parts[1] if len(parts) > 1 else ""
                
                # Check country codes
                if countries:
                    if any(country.upper() in name.upper() for country in countries):
                        filtered.append(config)
                        continue
                
                # Check name contains
                if name_contains and name_contains.lower() in name.lower():
                    filtered.append(config)
            
            configs = filtered
        
        return configs
    
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
    def _save_subscriptions(self):
        """Save all subscriptions to storage directory."""
        for subscription in self.subscriptions.values():
            self._save_subscription(subscription)
    
    def _save_subscription(self, subscription: Subscription):
        """Save a single subscription to storage directory."""
        try:
            file_path = os.path.join(self.storage_dir, f"{subscription.id}.json")
            data = subscription.to_dict()
            # Include configs in the saved data
            data['configs'] = subscription.configs
            
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False, indent=2)
        except Exception as e:
            logger.error(f"Failed to save subscription {subscription.name}: {str(e)}")
