.. _troubleshooting:

Troubleshooting
===============

This section covers common issues when using V2ROOT and how to resolve them.

Installation Issues
------------------

**Problem**: ``pip install v2root`` fails with "No matching distribution found".

**Solution**:
- Ensure you have Python 3.8 or higher:

  .. code-block:: bash

     python --version

- Check your internet connection or try a different PyPI mirror:

  .. code-block:: bash

     pip install v2root --index-url https://pypi.org/simple

**Problem**: ``libv2root.dll`` or ``libv2root.so`` not found after installation.

**Solution**:
- Verify that the shared library is included in the package:

  .. code-block:: bash

     pip show v2root
     ls -l $(python -c "import v2root; print(v2root.__path__[0])")/lib

- If missing, reinstall the package or manually place the library in the ``lib/`` directory.

Compilation Issues
-----------------

**Problem**: Compilation of ``libv2root.dll`` fails with "command not found" for ``x86_64-w64-mingw32-gcc``.

**Solution**:
- Ensure MSYS2 is installed and the MinGW64 environment is set up:

  .. code-block:: bash

     pacman -S mingw-w64-x86_64-gcc

- Add MinGW64 to your PATH:

  .. code-block:: bash

     export PATH=$PATH:/mingw64/bin

- Compile again using the correct compiler.

Runtime Issues
--------------

**Problem**: ``start_proxy`` fails with "Invalid configuration string".

**Solution**:
- Check the configuration string format (see :ref:`supported_options`).
- Example valid VLESS string: ``vless://uuid@example.com:443?type=tcp&security=tls&sni=example.com``

**Problem**: Proxy starts but no connection is established.

**Solution**:
- Verify the HTTP/SOCKS ports are not in use:

  .. code-block:: bash

     netstat -tuln | grep 10808

- Ensure the V2Ray executable is accessible and correctly configured (see :ref:`installation`).

Virtual Environment Issues
-------------------------

**Problem**: Activating virtual environment in PowerShell fails with a security error.

**Solution**:
- Change the execution policy:

  .. code-block:: bash

     Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned

- Or activate in CMD:

  .. code-block:: bash

     .\venv\Scripts\activate.bat

Error Code Issues
-----------------

This section explains common V2Ray error codes, their causes, and solutions. Always check the ``v2root.log`` file in your project directory for detailed error messages to identify the specific issue.

Error -1: General Error
~~~~~~~~~~~~~~~~~~~~~~~

**What is Error Code -1?**

Error Code -1 is a "General Error" in V2Root, meaning something unexpected prevented the program from completing your request. This is a catch-all error for issues like missing files, insufficient permissions, or system misconfigurations when processing your configuration string (e.g., ``vless://``, ``vmess://``, ``ss://``).

**Why Does This Happen?**

This error can occur due to:
- The configuration string is invalid or cannot be processed to create ``config.json``.
- On Linux, V2Ray is not installed or is outdated.
- The program lacks permissions to access files or network resources.
- The V2Root library file (``libv2root.dll`` on Windows, ``libv2root.so`` on Linux) is missing or inaccessible.
- Conflicts with antivirus or firewall software.

**How to Fix It**

Follow these steps carefully to resolve the issue:

1. **Validate the Configuration String**:
   - Ensure your configuration string starts with a supported protocol:
     - ``vless://``
     - ``vmess://``
     - ``ss://``
   - Example valid string:
     .. code-block:: none

        vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
   - Check for typos in the user ID, server address, or port. Contact your VPN provider for a correct string if unsure.

2. **Verify V2Ray Installation (Linux Only)**:
   - Open a terminal and check if V2Ray is installed:
     .. code-block:: bash

        v2ray --version
   - If you see a version number (e.g., ``V2Ray 5.12.1``), V2Ray is installed. If it’s outdated or missing, install the latest version.
   - For Ubuntu/Debian:
     .. code-block:: bash

        sudo apt update
        sudo apt install v2ray
   - For CentOS/RHEL:
     .. code-block:: bash

        sudo yum install v2ray
   - For other distros, download from the official V2Ray website and follow their installation instructions.

3. **Check V2Root Library File**:
   - On Windows, ensure ``libv2root.dll`` exists in the V2Root folder, for example:
     .. code-block:: none

        C:\V2Root\libv2root.dll
     - Replace ``C:\V2Root\`` with the actual folder where you installed V2Root.
     - Check if it exists:
       .. code-block:: powershell

          dir C:\V2Root\libv2root.dll
     - If missing, reinstall V2Root or contact support.
   - On Linux, ensure ``libv2root.so`` exists, for example:
     .. code-block:: none

        /usr/local/lib/v2root/libv2root.so
     - Replace ``/usr/local/lib/v2root/`` with the actual V2Root folder path.
     - Check if it exists:
       .. code-block:: bash

          ls /usr/local/lib/v2root/libv2root.so
     - Ensure it’s readable:
       .. code-block:: bash

          chmod +r /usr/local/lib/v2root/libv2root.so
     - If missing, contact support.

4. **Run as Administrator**:
   - On Windows:
     - Right-click ``v2root.py`` and select "Run as administrator".
     - Or open PowerShell as admin and run:
       .. code-block:: powershell

          python v2root.py
   - On Linux:
     - Use ``sudo`` to run the script:
       .. code-block:: bash

          sudo python3 v2root.py
     - If you get a permission error, ensure the script is executable:
       .. code-block:: bash

          chmod +x v2root.py

5. **Inspect the Log File**:
   - Open ``v2root.log`` in the same folder as ``v2root.py`` using a text editor.
   - Look for errors like:
     - "Invalid configuration string" (check your configuration string).
     - "File not found" (check ``libv2root.dll`` or ``libv2root.so``).
     - "Permission denied" (run as admin or fix permissions).
     - "V2Ray core not found" (Linux: install V2Ray).
   - On Linux, view the log:
     .. code-block:: bash

        cat v2root.log

6. **Check for Software Conflicts**:
   - Ensure no antivirus or firewall blocks V2Root.
   - On Windows, add exceptions for ``v2root.py`` and ``C:\V2Root\libv2root.dll`` (replace with your path) in Windows Defender:
     - Settings > Update & Security > Windows Security > Virus & Threat Protection > Manage Settings > Exclusions > Add an exclusion.
   - On Linux, check if ``ufw`` blocks ports 10808 or 1080:
     .. code-block:: bash

        sudo ufw status
     - Allow them:
       .. code-block:: bash

          sudo ufw allow 10808
          sudo ufw allow 1080

7. **Verify System Requirements**:
   - Ensure your system meets V2Root’s requirements:
     - Windows 7 or later (64-bit recommended).
     - Linux: Ubuntu 18.04+, CentOS 7+, Arch, Fedora, or compatible.
     - At least 2GB RAM and 500MB free disk space.
   - Update your system:
     - Windows: Run Windows Update.
     - Linux:
       .. code-block:: bash

          sudo apt update && sudo apt upgrade

Error -2: Service Error
~~~~~~~~~~~~~~~~~~~~~~~

**What is Error Code -2?**

Error Code -2 is a "Service Error" in V2Root, indicating that the V2Ray program failed to start or couldn’t connect to the internet after processing your configuration string. This error relates to issues with launching the V2Ray core or establishing network connectivity.

**Why Does This Happen?**

Common causes include:
- On Linux, V2Ray is not installed or is outdated, preventing the service from starting.
- Network ports (e.g., 10808, 1080) are already in use by another program.
- A firewall or antivirus is blocking V2Ray’s network access.
- Internet connectivity issues (e.g., no internet or server downtime).
- The configuration string caused an invalid ``config.json`` to be generated.

**How to Fix It**

Follow these detailed steps to resolve the issue:

1. **Validate the Configuration String**:
   - Ensure your configuration string starts with:
     - ``vless://``
     - ``vmess://``
     - ``ss://``
   - Example valid string:
     .. code-block:: none

        vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
   - Check for typos in the user ID, server address, or port. Contact your VPN provider for a correct string.

2. **Verify V2Ray Installation (Linux Only)**:
   - Check if V2Ray is installed:
     .. code-block:: bash

        v2ray --version
   - If missing or outdated, install the latest version:
     - For Ubuntu/Debian:
       .. code-block:: bash

          sudo apt update
          sudo apt install v2ray
     - For CentOS/RHEL:
       .. code-block:: bash

          sudo yum install v2ray
     - For other distros, download from the official V2Ray website.

3. **Check V2Root Library File**:
   - On Windows, ensure ``libv2root.dll`` exists, e.g.:
     .. code-block:: none

        C:\V2Root\libv2root.dll
     - Replace ``C:\V2Root\`` with your V2Root folder path.
     - Check:
       .. code-block:: powershell

          dir C:\V2Root\libv2root.dll
     - If missing, contact support.
   - On Linux, ensure ``libv2root.so`` exists, e.g.:
     .. code-block:: none

        /usr/local/lib/v2root/libv2root.so
     - Replace ``/usr/local/lib/v2root/`` with your path.
     - Check:
       .. code-block:: bash

          ls /usr/local/lib/v2root/libv2root.so
     - Ensure readable:
       .. code-block:: bash

          chmod +r /usr/local/lib/v2root/libv2root.so
     - If missing, contact support.

4. **Check Port Availability**:
   - Ensure ports 10808 (HTTP) and 1080 (SOCKS) are free:
     - On Linux:
       .. code-block:: bash

          netstat -tuln | grep 10808
          netstat -tuln | grep 1080
       - If in use, find the program:
         .. code-block:: bash

            sudo lsof -i :10808
       - Stop it or change ports in your script.
     - On Windows:
       .. code-block:: powershell

          netstat -an | findstr 10808
          netstat -an | findstr 1080
       - If used, close the program via Task Manager or change ports.

5. **Test Internet Connectivity**:
   - Check internet:
     .. code-block:: bash

        ping 8.8.8.8
   - If no response, restart your router or contact your ISP.
   - Test the server in your configuration string:
     .. code-block:: bash

        ping server-address
     - Replace ``server-address`` with the address from your string. If it fails, contact your VPN provider.

6. **Check Firewall and Antivirus**:
   - On Windows, allow ``v2root.py`` and ``C:\V2Root\libv2root.dll`` (use your path) in Windows Defender:
     - Settings > Update & Security > Windows Security > Virus & Threat Protection > Manage Settings > Exclusions.
   - On Linux, allow ports:
     .. code-block:: bash

        sudo ufw allow 10808
        sudo ufw allow 1080
        sudo ufw status

7. **Inspect the Log File**:
   - Open ``v2root.log`` and look for:
     - "Port already in use" (free ports).
     - "Network unreachable" (check internet/server).
     - "Invalid config" (check configuration string).
   - View log:
     .. code-block:: bash

        cat v2root.log

Error -3: Config Error
~~~~~~~~~~~~~~~~~~~~~~

**What is Error Code -3?**

Error Code -3 is a "Config Error" in V2Root, meaning the V2Ray configuration string you provided (e.g., ``vless://``, ``vmess://``, ``ss://``) is incorrect, malformed, or incompatible. This prevents V2Root from generating a valid ``config.json`` file, which stops V2Ray from starting or connecting properly.

**Why Does This Happen?**

This error can occur because:
- The configuration string has an invalid format or missing components (e.g., wrong protocol, missing user ID, or incorrect server address).
- The server address, port, or user ID in the configuration string is wrong or outdated.
- The V2Ray protocol in the string (e.g., ``vless``, ``vmess``) is not supported by your version of V2Ray or V2Root.
- The program failed to parse the configuration string due to a bug or unsupported characters.
- The VPN provider gave you an incorrect or expired configuration string.

**How to Fix It**

Follow these detailed steps to resolve the issue:

1. **Validate the Configuration String**:
   - Ensure your configuration string starts with a supported protocol:
     - ``vless://``
     - ``vmess://``
     - ``ss://``
   - Example of a valid string:
     .. code-block:: none

        vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
   - Check for:
     - **User ID**: A valid UUID or identifier (e.g., ``123e4567-e89b-12d3-a456-426614174000``).
     - **Server Address**: A correct IP address (e.g., ``192.168.1.1``) or domain (e.g., ``vpn.example.com``).
     - **Port**: A number like ``443`` or ``1080``.
     - **Parameters**: Correct settings like ``security=tls`` or ``type=tcp``.
   - Compare your string with examples from your VPN provider. Fix any typos or missing parts.

2. **Contact Your VPN Provider**:
   - If you’re unsure about the configuration string, send it to your VPN provider (remove sensitive parts like the user ID if needed) and ask them to verify:
     - Is the protocol (``vless``, ``vmess``, ``ss``) correct?
     - Is the server address and port still active?
     - Are the user ID and other parameters valid?
   - Request a new configuration string if yours is outdated or incorrect.

3. **Test the Server Address**:
   - Extract the server address from your configuration string (e.g., ``server-address`` in ``vless://user-id@server-address:443``).
   - Test if it’s reachable:
     .. code-block:: bash

        ping server-address
   - If the ping fails, the server may be down or the address is wrong. Ask your VPN provider for an alternative server.

4. **Test a Different Configuration String**:
   - Ask your VPN provider for another configuration string (e.g., a different server or protocol).
   - Update your script to use the new string (e.g., pass it to ``set_config_string``) and rerun ``v2root.py``.

5. **Update V2Ray (Linux Only)**:
   - An outdated V2Ray version may not support newer protocols in the configuration string. Check the version:
     .. code-block:: bash

        v2ray --version
   - If it’s missing or old, update or install V2Ray:
     - For Ubuntu/Debian:
       .. code-block:: bash

          sudo apt update
          sudo apt install v2ray
     - For CentOS/RHEL:
       .. code-block:: bash

          sudo yum install v2ray
     - For other distros, download from the official V2Ray website.

6. **Check Windows Setup**:
   - On Windows, V2Ray is bundled with V2Root, so no separate installation is needed.
   - Ensure the file ``libv2root.dll`` is in the same folder as ``v2root.py`` or in the ``lib/build_win`` subdirectory.
   - If missing, redownload V2Root from the official V2Root release page.

7. **Inspect the Log File**:
   - Open ``v2root.log`` in the same folder as ``v2root.py`` with a text editor (e.g., Notepad on Windows, ``nano`` on Linux).
   - Look for errors related to the configuration string or ``config.json`` generation, such as:
     - "Invalid configuration string" (check string format).
     - "Failed to parse JSON" (indicates V2Root couldn’t create a valid ``config.json`` from the string).
     - "Unknown protocol" (use a supported protocol like ``vless`` or ``vmess``).
     - "Server rejected" (wrong user ID, server address, or port).
   - View the log on Linux:
     .. code-block:: bash

        cat v2root.log
   - If the log mentions ``config.json``, it may show the generated file’s contents. Check for errors like missing fields or invalid JSON syntax.

8. **Reinstall V2Root**:
   - If the program is failing to process the configuration string, there might be a bug or corrupted files.
   - Delete the V2Root folder and redownload the latest version from the official V2Root release page.
   - Extract and rerun ``v2root.py`` with your configuration string.

Error -4: Connection Error
~~~~~~~~~~~~~~~~~~~~~~~~~~

**What is Error Code -4?**

Error Code -4 is a "Connection Error" in V2Root, meaning the program couldn’t connect to the V2Ray server specified in your configuration string or use the network. This error occurs after V2Root generates ``config.json`` from the string and tries to establish a connection, indicating network or server issues.

**Why Does This Happen?**

Possible causes include:
- The server address or port in the configuration string is incorrect or outdated.
- The V2Ray server is down, unreachable, or rejecting connections.
- Network ports (e.g., 10808 for HTTP, 1080 for SOCKS) are blocked by a firewall, router, or ISP.
- No internet connection or an unstable network.
- Other VPN or proxy software is interfering with V2Root’s network access.
- The generated ``config.json`` has issues due to errors in the configuration string processing.

**How to Fix It**

Follow these detailed steps to resolve the issue:

1. **Verify Server Address and Port in the Configuration String**:
   - Check your configuration string (e.g., ``vless://user-id@server-address:443?security=tls&type=tcp#MyVPN``).
   - Ensure the server address (e.g., ``server-address``) and port (e.g., ``443``) are correct.
   - Test the server’s reachability:
     .. code-block:: bash

        ping server-address
   - If the ping fails, the server may be down or the address is wrong. Contact your VPN provider to confirm the server details or get a new configuration string.

2. **Test a Different Configuration String**:
   - Ask your VPN provider for an alternative configuration string with a different server or port.
   - Update your script to use the new string (e.g., pass it to ``set_config_string``) and rerun ``v2root.py``.

3. **Check Internet Connectivity**:
   - Ensure your internet is working:
     .. code-block:: bash

        ping 8.8.8.8
   - If there’s no response, troubleshoot your network:
     - Restart your router or modem.
     - Check Wi-Fi or Ethernet connection.
     - Contact your ISP if the issue persists.

4. **Verify Port Availability**:
   - V2Root uses ports 10808 (HTTP) and 1080 (SOCKS) by default for local connections. Ensure they’re not blocked or in use.
   - On Linux:
     .. code-block:: bash

        netstat -tuln | grep 10808
        netstat -tuln | grep 1080
     - If ports are in use, identify the program:
       .. code-block:: bash

          sudo lsof -i :10808
     - Stop the conflicting program or modify your script to use different ports.
   - On Windows:
     .. code-block:: powershell

        netstat -an | findstr 10808
        netstat -an | findstr 1080
     - If ports are used, find the program in Task Manager and close it, or change ports in your script.

5. **Check Firewall and Antivirus Settings**:
   - On Windows, ensure Windows Defender or other antivirus allows ``v2root.py`` and ``libv2root.dll``:
     - Go to Settings > Update & Security > Windows Security > Virus & Threat Protection > Manage Settings > Exclusions > Add an exclusion for both files.
   - On Linux, check if ``ufw`` or another firewall is blocking ports:
     .. code-block:: bash

        sudo ufw status
     - Allow V2Root’s ports:
       .. code-block:: bash

          sudo ufw allow 10808
          sudo ufw allow 1080
   - Check your router’s firewall settings. If behind NAT, ensure ports 10808 and 1080 are forwarded.
   - If your ISP blocks VPN ports, ask your VPN provider for a configuration string using a different port (e.g., 443).

6. **Inspect the Log File**:
   - Open ``v2root.log`` in the same folder as ``v2root.py`` with a text editor.
   - Look for errors related to the connection or ``config.json`` generation, such as:
     - "Connection refused" (wrong server address/port or server down).
     - "Network timeout" (internet issue or server unreachable).
     - "Failed to parse config" (indicates an issue with the generated ``config.json`` due to the configuration string).
   - View the log on Linux:
     .. code-block:: bash

        cat v2root.log
   - If the log shows ``config.json`` errors, it may indicate the configuration string was malformed. Double-check the string with your VPN provider.

7. **Check for VPN/Proxy Conflicts**:
   - Ensure no other VPN or proxy software (e.g., OpenVPN, NordVPN) is running, as they may interfere with V2Root.
   - On Windows, disable other VPNs:
     - Settings > Network & Internet > VPN > Disconnect any active VPNs.
   - On Linux, stop other VPN services:
     .. code-block:: bash

        sudo systemctl stop openvpn

8. **Update V2Root and V2Ray**:
   - On Linux, ensure V2Ray is up-to-date to handle the configuration string and network protocols:
     .. code-block:: bash

        v2ray --version
        sudo apt update
        sudo apt install v2ray
   - On Windows, ensure you have the latest V2Root version:
     - Delete the V2Root folder and redownload from the official V2Root release page.
     - Verify ``libv2root.dll`` is present in the same folder as ``v2root.py`` or in ``lib/build_win``.

Error -5: Initialization Error
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**What is Error Code -5?**

Error Code -5 is an "Initialization Error" in V2Root, meaning the program couldn’t start V2Ray due to missing or misconfigured components when processing your configuration string. This occurs during the setup phase.

**Why Does This Happen?**

Possible causes include:
- On Linux, V2Ray is not installed system-wide.
- The V2Root library (``libv2root.dll`` on Windows, ``libv2root.so`` on Linux) is missing or inaccessible.
- Missing system libraries (e.g., ``libjansson`` on Linux).
- Insufficient permissions for V2Root files.
- Incompatible system environment.

**How to Fix It**

Follow these steps:

1. **Validate the Configuration String**:
   - Ensure it starts with:
     - ``vless://``
     - ``vmess://``
     - ``ss://``
   - Example:
     .. code-block:: none

        vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
   - Check for typos. Contact your VPN provider for a valid string.

2. **Verify V2Ray Installation (Linux Only)**:
   - Check:
     .. code-block:: bash

        v2ray --version
   - If missing or outdated, install the latest V2Ray:
     - For Ubuntu/Debian:
       .. code-block:: bash

          sudo apt update
          sudo apt install v2ray
     - For CentOS/RHEL:
       .. code-block:: bash

          sudo yum install v2ray
     - For other distros, download from the official V2Ray website.

3. **Check V2Root Library File**:
   - Windows: Ensure ``libv2root.dll`` exists, e.g.:
     .. code-block:: none

        C:\V2Root\libv2root.dll
     - Replace ``C:\V2Root\`` with your path.
     - Check:
       .. code-block:: powershell

          dir C:\V2Root\libv2root.dll
     - If missing, contact support.
   - Linux: Ensure ``libv2root.so`` exists, e.g.:
     .. code-block:: none

        /usr/local/lib/v2root/libv2root.so
     - Replace path.
     - Check:
       .. code-block:: bash

          ls /usr/local/lib/v2root/libv2root.so
     - Ensure readable:
       .. code-block:: bash

          chmod +r /usr/local/lib/v2root/libv2root.so
     - If missing, contact support.

4. **Check for Missing Libraries (Linux Only)**:
   - Verify dependencies for ``libv2root.so``:
     .. code-block:: bash

        ldd /usr/local/lib/v2root/libv2root.so
     - Replace the path with your ``libv2root.so`` location.
   - Install missing libraries, e.g.:
     .. code-block:: bash

        sudo apt install libjansson-dev libc6

5. **Verify File Permissions**:
   - Linux: Ensure ``v2root.py`` and ``libv2root.so`` are accessible:
     .. code-block:: bash

        ls -l v2root.py
        ls -l /usr/local/lib/v2root/libv2root.so
        chmod +x v2root.py
        chmod +r /usr/local/lib/v2root/libv2root.so
     - Use your ``libv2root.so`` path.
   - Windows: Ensure ``v2root.py`` and ``C:\V2Root\libv2root.dll`` aren’t blocked:
     - Right-click > Properties > Unblock (if visible).

6. **Inspect the Log File**:
   - Open ``v2root.log`` for errors like:
     - "V2Ray core not found" (install V2Ray).
     - "Library not found" (check ``libv2root.dll`` or ``libv2root.so``).
     - "Invalid config" (check configuration string).
   - View:
     .. code-block:: bash

        cat v2root.log

7. **Update System**:
   - Windows: Run Windows Update.
   - Linux:
     .. code-block:: bash

        sudo apt update && sudo apt upgrade

Error -6: Proxy Error
~~~~~~~~~~~~~~~~~~~~~

**What is Error Code -6?**

Error Code -6 is a "Proxy Error" in V2Root, meaning the program couldn’t set or clear your system’s proxy settings after processing your configuration string. This affects V2Root’s ability to configure V2Ray as a proxy.

**Why Does This Happen?**

Causes include:
- Insufficient permissions to modify proxy settings.
- Other VPN/proxy software interfering.
- Corrupted or locked system proxy settings.
- Antivirus blocking proxy changes.
- Issues with the generated ``config.json`` from the configuration string.

**How to Fix It**

Follow these steps:

1. **Validate the Configuration String**:
   - Ensure it starts with:
     - ``vless://``
     - ``vmess://``
     - ``ss://``
   - Example:
     .. code-block:: none

        vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
   - Check typos. Contact your VPN provider for a valid string.

2. **Run as Administrator**:
   - Windows: Right-click ``v2root.py`` > "Run as administrator".
     - Or PowerShell as admin:
       .. code-block:: powershell

          python v2root.py
   - Linux:
     .. code-block:: bash

        sudo python3 v2root.py
     - Ensure executable:
       .. code-block:: bash

          chmod +x v2root.py

3. **Check V2Root Library File**:
   - Windows: Ensure ``libv2root.dll``, e.g.:
     .. code-block:: none

        C:\V2Root\libv2root.dll
     - Replace ``C:\V2Root\`` with your path.
     - Check:
       .. code-block:: powershell

          dir C:\V2Root\libv2root.dll
     - If missing, contact support.
   - Linux: Ensure ``libv2root.so``, e.g.:
     .. code-block:: none

        /usr/local/lib/v2root/libv2root.so
     - Replace path.
     - Check:
       .. code-block:: bash

          ls /usr/local/lib/v2root/libv2root.so
     - Ensure readable:
       .. code-block:: bash

          chmod +r /usr/local/lib/v2root/libv2root.so
     - If missing, contact support.

4. **Check for Conflicting Software**:
   - Close other VPNs/proxies.
   - Windows: Settings > Network & Internet > Proxy > No manual proxy unless needed.
   - Linux:
     .. code-block:: bash

        sudo systemctl stop openvpn

5. **Reset Proxy Settings**:
   - Windows:
     .. code-block:: powershell

        netsh winhttp reset proxy
     - Restart PC.
   - Linux:
     .. code-block:: bash

        gsettings reset org.gnome.system.proxy

6. **Check Antivirus**:
   - Windows: Add exceptions for ``v2root.py`` and ``C:\V2Root\libv2root.dll`` (your path) in Windows Defender.
   - Linux: Ensure no security software blocks V2Root.

7. **Inspect the Log File**:
   - Open ``v2root.log`` for:
     - "Permission denied" (run as admin).
     - "Proxy setting failed" (check conflicts).
     - "Invalid config" (check configuration string).
   - View:
     .. code-block:: bash

        cat v2root.log

Unknown Error Codes
~~~~~~~~~~~~~~~~~~~

**What are Unknown Error Codes?**

Unknown Error Codes in V2Root are any errors not explicitly identified as -1 through -6. These are unexpected issues that may arise due to bugs, system incompatibilities, or unique configurations not handled by V2Root’s error reporting.

**Why Does This Happen?**

Possible causes include:
- A bug in V2Root or V2Ray.
- System-specific issues (e.g., outdated OS, missing dependencies).
- Conflicts with other software or drivers.
- Invalid or corrupted configuration string causing unhandled errors.
- Hardware or network issues not detected by V2Root.

**How to Fix It**

Follow these steps to troubleshoot:

1. **Inspect the Log File**:
   - Open ``v2root.log`` in the same folder as ``v2root.py`` with a text editor.
   - Look for detailed error messages, such as:
     - Specific error codes or messages not listed as -1 to -6.
     - Stack traces or system errors (e.g., memory issues, library failures).
     - Messages about configuration or network failures.
   - View the log on Linux:
     .. code-block:: bash

        cat v2root.log
   - Note any specific errors for support.

2. **Validate the Configuration String**:
   - Ensure it starts with:
     - ``vless://``
     - ``vmess://``
     - ``ss://``
   - Example:
     .. code-block:: none

        vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
   - Check typos and contact your VPN provider for a valid string.

3. **Verify V2Ray Installation (Linux Only)**:
   - Check:
     .. code-block:: bash

        v2ray --version
   - If missing or outdated, install the latest V2Ray:
     - For Ubuntu/Debian:
       .. code-block:: bash

          sudo apt update
          sudo apt install v2ray
     - For CentOS/RHEL:
       .. code-block:: bash

          sudo yum install v2ray
     - For other distros, download from the official V2Ray website.

4. **Check V2Root Library File**:
   - Windows: Ensure ``libv2root.dll`` exists, e.g.:
     .. code-block:: none

        C:\V2Root\libv2root.dll
     - Replace ``C:\V2Root\`` with your path.
     - Check:
       .. code-block:: powershell

          dir C:\V2Root\libv2root.dll
     - If missing, contact support.
   - Linux: Ensure ``libv2root.so`` exists, e.g.:
     .. code-block:: none

        /usr/local/lib/v2root/libv2root.so
     - Replace path.
     - Check:
       .. code-block:: bash

          ls /usr/local/lib/v2root/libv2root.so
     - Ensure readable:
       .. code-block:: bash

          chmod +r /usr/local/lib/v2root/libv2root.so
     - If missing, contact support.

5. **Restart the Program**:
   - Close V2Root and rerun ``v2root.py``:
     - Windows:
       .. code-block:: powershell

          python v2root.py
     - Linux:
       .. code-block:: bash

          python3 v2root.py
   - If the error persists, try restarting your computer.

6. **Update System and Software**:
   - Windows: Run Windows Update to ensure all system components are current.
   - Linux:
     .. code-block:: bash

        sudo apt update && sudo apt upgrade
   - Reinstall V2Root if issues persist:
     - Delete the V2Root folder and redownload from the official V2Root release page.

7. **Check for Conflicts**:
   - Ensure no other VPN, proxy, or network software is running.
   - Windows: Settings > Network & Internet > VPN > Disconnect any active VPNs.
   - Linux:
     .. code-block:: bash

        sudo systemctl stop openvpn

Additional Resources
--------------------

- Contact support via [Telegram (@Sepehr0Day)](https://t.me/Sepehr0Day) or [GitHub Issues](https://github.com/V2RayRoot/V2Root/issues).
- Include your script (e.g., ``v2root.py``), configuration string (remove sensitive parts), ``v2root.log``, and OS when reporting issues.