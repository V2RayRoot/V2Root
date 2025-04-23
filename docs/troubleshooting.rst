.. _troubleshooting:

Troubleshooting
===============

This section covers common issues when using V2ROOT and how to resolve them.

Installation Issues
-------------------

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
------------------

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
--------------------------

**Problem**: Activating virtual environment in PowerShell fails with a security error.

**Solution**:
- Change the execution policy:

  .. code-block:: bash

     Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned

- Or activate in CMD:

  .. code-block:: bash

     .\venv\Scripts\activate.bat