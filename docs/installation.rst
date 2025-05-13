.. _installation:

Installation
============

This section covers the installation process for the V2ROOT library on both Linux and Windows platforms.

Prerequisites
-------------

Before installing V2ROOT, ensure you have the following:

- **Python 3.6 or higher**: V2ROOT requires Python 3.6 or later. You can check your Python version with:

  .. code-block:: bash

     python3 --version

- **V2Ray**: V2ROOT relies on the V2Ray core to function. This should already be installed and properly configured on your system.

Installing V2ROOT
-----------------

V2ROOT can be easily installed using ``pip`` on both Linux and Windows.

Windows Installation
~~~~~~~~~~~~~~~~~~~~

On Windows, assuming all prerequisites (like V2Ray and required DLLs) are already installed, simply install V2ROOT using ``pip``:

.. code-block:: bash

   pip install v2root

This will download and install the V2ROOT package along with its dependencies (e.g., ``colorama`` for colored terminal output).

Linux Installation
~~~~~~~~~~~~~~~~~~

On Linux, you can also install V2ROOT using ``pip``:

.. code-block:: bash

   pip install v2root

However, since V2ROOT relies on the V2Ray core, you may encounter issues if V2Ray is not properly installed or configured. If you run into errors (e.g., the "General Failure" error with code ``-1``), follow the troubleshooting steps below.

Troubleshooting on Linux
~~~~~~~~~~~~~~~~~~~~~~~~

If you encounter the error ``Connection test failed: General failure (e.g., file operation failed, fork failed, or system command error)`` (error code ``-1``), it typically means V2ROOT cannot find or execute the V2Ray core. Follow these steps to resolve the issue:

1. **Check the V2Ray Executable Path**:

   V2ROOT expects the V2Ray executable (``v2ray``) to be located in the ``lib/`` directory of the package, typically at:

   .. code-block:: bash

      /home/<your-username>/.local/lib/python3.10/site-packages/v2root/lib/v2ray

   Verify that the ``v2ray`` executable exists and is executable:

   .. code-block:: bash

      ls -l /home/<your-username>/.local/lib/python3.10/site-packages/v2root/lib/v2ray

   If the file is missing or not executable, you’ll need to place the V2Ray core there. Proceed to the next step.

2. **Manually Download and Place the V2Ray Core**:

   If the V2Ray executable is missing or not functional, download it manually:

   .. code-block:: bash

      wget https://github.com/v2fly/v2ray-core/releases/download/v5.15.1/v2ray-linux-64.zip
      unzip v2ray-linux-64.zip -d /tmp/v2ray
      cp /tmp/v2ray/v2ray /home/<your-username>/.local/lib/python3.10/site-packages/v2root/lib/v2ray
      chmod +x /home/<your-username>/.local/lib/python3.10/site-packages/v2root/lib/v2ray

   Replace ``<your-username>`` with your actual username.

3. **Test the V2Ray Core**:

   Verify that the V2Ray core is functional by running:

   .. code-block:: bash

      /home/<your-username>/.local/lib/python3.10/site-packages/v2root/lib/v2ray --version

   If this command outputs the V2Ray version (e.g., ``V2Ray 5.15.1``), the core is working correctly. If you get an error, check for missing dependencies:

   .. code-block:: bash

      ldd /home/<your-username>/.local/lib/python3.10/site-packages/v2root/lib/v2ray

   If any libraries are missing (e.g., ``libjansson.so`` or ``libssl.so``), install them:

   .. code-block:: bash

      sudo apt install -y libjansson-dev libssl-dev

4. **Verify Installation**:

   After resolving any issues, test that V2ROOT can initialize properly by running a simple script:

   .. code-block:: python

      from v2root import V2ROOT

      client = V2ROOT()
      print("V2ROOT initialized successfully!")

   If you still encounter errors, check the V2Ray logs for more details. The log files are created in the same directory as the script you are running. For example, if you run a script named ``my_script.py``, the logs will be created as ``v2ray_service.log``, ``v2ray.log``, and ``v2ray_err.log`` in the same directory as ``my_script.py``. You can view them with:

   .. code-block:: bash

      cat v2root.log