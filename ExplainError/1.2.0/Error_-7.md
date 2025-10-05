# Error Code -7: Process Already Running

## English

### What is Error Code -7?
Error Code -7 is a "Process Already Running" error in V2Root, meaning you tried to start a new V2Ray connection while another V2Ray process is still active. V2Root prevents multiple instances from running simultaneously to avoid conflicts with network ports, system resources, and proxy settings.

### Why Does This Happen?
This error occurs when:
- You previously started V2Ray and forgot to stop it before starting a new connection.
- A previous V2Ray process didn't terminate properly (e.g., due to a crash or forceful program closure).
- The process ID (PID) is stored in the system registry (Windows) but the actual process might still be running in the background.
- You're running multiple scripts or programs that try to start V2Ray simultaneously.
- On Windows, the registry contains a PID from a previous session that wasn't cleaned up.

### How to Fix It
Follow these detailed steps to resolve the issue:

1. **Stop the Existing V2Ray Process**:
   - The safest way is to use V2Root's built-in stop function in your script:
     ```python
     v2root = V2ROOT()
     v2root.stop()
     ```
   - This will properly terminate the V2Ray process and clean up proxy settings.

2. **Check for Running V2Ray Processes**:
   - On Windows:
     - Open Task Manager (press `Ctrl+Shift+Esc`).
     - Look for processes named `v2ray.exe` or `v2ray`.
     - Right-click and select "End Task" to terminate them.
     - Alternatively, use PowerShell:
       ```powershell
       tasklist | findstr v2ray
       taskkill /IM v2ray.exe /F
       ```
   - On Linux:
     - Open a terminal and check for V2Ray processes:
       ```bash
       ps aux | grep v2ray
       ```
     - Kill the process using its PID (replace `12345` with the actual PID):
       ```bash
       sudo kill -9 12345
       ```
     - Or kill all V2Ray processes:
       ```bash
       sudo pkill v2ray
       ```

3. **Clear Registry Data (Windows Only)**:
   - V2Root stores the PID in the Windows Registry under `HKEY_CURRENT_USER\Software\V2ROOT`.
   - Open Registry Editor:
     - Press `Win+R`, type `regedit`, and press Enter.
     - Navigate to: `HKEY_CURRENT_USER\Software\V2ROOT`
     - Look for a value named `V2RayPID`.
     - Right-click and delete it.
   - Alternatively, use PowerShell:
     ```powershell
     Remove-ItemProperty -Path "HKCU:\Software\V2ROOT" -Name "V2RayPID" -ErrorAction SilentlyContinue
     ```

4. **Wait a Few Seconds**:
   - After stopping the process, wait 5-10 seconds before starting V2Ray again.
   - This ensures the process fully terminates and releases network resources.

5. **Restart Your Script**:
   - After stopping the existing process, run your script again:
     ```bash
     python test.py
     ```
   - V2Root should now start successfully without the -7 error.

6. **Check for Port Conflicts**:
   - The previous V2Ray process might still be holding ports 2300 (HTTP) or 2301 (SOCKS).
   - On Windows:
     ```powershell
     netstat -an | findstr 2300
     netstat -an | findstr 2301
     ```
     - If ports are in use, find and terminate the program using Task Manager.
   - On Linux:
     ```bash
     netstat -tuln | grep 2300
     netstat -tuln | grep 2301
     ```
     - If ports are occupied, find the process:
       ```bash
       sudo lsof -i :2300
       sudo kill -9 <PID>
       ```

7. **Inspect the Log File**:
   - Open `v2root.log` in the same folder as your script.
   - Look for entries related to process management:
     - "V2Ray process already running with PID: ..." (note the PID and terminate it).
     - "Failed to stop V2Ray process" (indicates the process is stuck).
   - View the log:
     ```bash
     cat v2root.log
     ```

8. **Restart Your Computer (Last Resort)**:
   - If the process is stuck and won't terminate, restart your computer.
   - This will forcefully close all processes and clear any registry or system locks.

### How to Avoid This Error in the Future
To prevent Error -7 from happening again:

1. **Always Stop V2Ray Before Starting a New Connection**:
   - Add a stop command before starting:
     ```python
     v2root = V2ROOT()
     v2root.stop()  # Stop any existing process
     v2root.set_config_string(config_str)
     v2root.start()  # Start with new configuration
     ```

2. **Use Try-Finally Blocks**:
   - Ensure V2Ray stops even if your script crashes:
     ```python
     v2root = V2ROOT()
     try:
         v2root.set_config_string(config_str)
         v2root.start()
         # Your code here
     finally:
         v2root.stop()  # Always stop when done
     ```

3. **Check Process Status Before Starting**:
   - Verify no V2Ray process is running before starting a new one:
     ```python
     import subprocess
     
     # Check for existing process
     result = subprocess.run(['tasklist'], capture_output=True, text=True)  # Windows
     # result = subprocess.run(['ps', 'aux'], capture_output=True, text=True)  # Linux
     
     if 'v2ray' in result.stdout.lower():
         print("V2Ray is already running. Stopping it first...")
         v2root.stop()
     ```

4. **Don't Run Multiple Scripts Simultaneously**:
   - Avoid running multiple Python scripts that use V2Root at the same time.
   - If you need to test multiple configurations, stop V2Ray between tests.

### Still Stuck?
If you've tried all the steps and the issue persists, we're here to help! Contact us with the following details:
- The script file you're running (e.g., `test.py`).
- The `v2root.log` file from the same folder.
- Your operating system (Windows or Linux).
- A description of when the error started (e.g., "after a crash" or "when running multiple scripts").
- Report the issue on:
  - Telegram: @Sepehr0Day
  - GitHub: https://github.com/V2RayRoot/V2Root/issues

---

## Persian (فارسی)

### خطای کد -۷ چیست؟
خطای کد -۷ یه "خطای پروسه در حال اجرا" توی V2Rootه، یعنی شما سعی کردید یه اتصال جدید V2Ray راه‌اندازی کنید در حالی که یه پروسه V2Ray دیگه هنوز فعاله. V2Root از اجرای چندین نمونه به‌طور همزمان جلوگیری می‌کنه تا از تداخل با پورت‌های شبکه، منابع سیستم و تنظیمات پراکسی جلوگیری کنه.

### چرا این خطا رخ می‌ده؟
این خطا زمانی پیش میاد که:
- قبلا V2Ray رو شروع کردید و فراموش کردید قبل از شروع اتصال جدید اون رو متوقف کنید.
- یه پروسه قبلی V2Ray به درستی خاتمه پیدا نکرده (مثلا به خاطر کرش یا بستن اجباری برنامه).
- شناسه پروسه (PID) توی رجیستری سیستم (ویندوز) ذخیره شده ولی پروسه واقعی هنوز ممکنه توی پس‌زمینه در حال اجرا باشه.
- دارید چندین اسکریپت یا برنامه رو اجرا می‌کنید که همزمان سعی می‌کنن V2Ray رو شروع کنن.
- توی ویندوز، رجیستری یه PID از یه جلسه قبلی داره که پاک نشده.

### چطور درستش کنیم؟
این مراحل رو با دقت دنبال کنید تا مشکل حل بشه:

1. **متوقف کردن پروسه موجود V2Ray**:
   - امن‌ترین راه استفاده از تابع توقف داخلی V2Root توی اسکریپتتونه:
     ```python
     v2root = V2ROOT()
     v2root.stop()
     ```
   - این کار پروسه V2Ray رو به درستی خاتمه می‌ده و تنظیمات پراکسی رو پاک می‌کنه.

2. **چک کردن پروسه‌های V2Ray در حال اجرا**:
   - توی ویندوز:
     - Task Manager رو باز کنید (`Ctrl+Shift+Esc`).
     - دنبال پروسه‌های با نام `v2ray.exe` یا `v2ray` بگردید.
     - راست‌کلیک کنید و "End Task" رو انتخاب کنید.
     - یا از پاورشل استفاده کنید:
       ```powershell
       tasklist | findstr v2ray
       taskkill /IM v2ray.exe /F
       ```
   - توی لینوکس:
     - ترمینال رو باز کنید و پروسه‌های V2Ray رو چک کنید:
       ```bash
       ps aux | grep v2ray
       ```
     - پروسه رو با PID اش بکشید (`12345` رو با PID واقعی عوض کنید):
       ```bash
       sudo kill -9 12345
       ```
     - یا همه پروسه‌های V2Ray رو بکشید:
       ```bash
       sudo pkill v2ray
       ```

3. **پاک کردن داده‌های رجیستری (فقط ویندوز)**:
   - V2Root شناسه PID رو توی رجیستری ویندوز زیر `HKEY_CURRENT_USER\Software\V2ROOT` ذخیره می‌کنه.
   - Registry Editor رو باز کنید:
     - `Win+R` رو فشار بدید، `regedit` رو تایپ کنید و Enter بزنید.
     - برید به: `HKEY_CURRENT_USER\Software\V2ROOT`
     - دنبال یه مقدار به اسم `V2RayPID` بگردید.
     - راست‌کلیک کنید و حذفش کنید.
   - یا از پاورشل استفاده کنید:
     ```powershell
     Remove-ItemProperty -Path "HKCU:\Software\V2ROOT" -Name "V2RayPID" -ErrorAction SilentlyContinue
     ```

4. **چند ثانیه صبر کنید**:
   - بعد از متوقف کردن پروسه، ۵-۱۰ ثانیه صبر کنید قبل از شروع دوباره V2Ray.
   - این کار اطمینان می‌ده پروسه کاملا خاتمه پیدا کرده و منابع شبکه رو آزاد کرده.

5. **ری‌استارت کردن اسکریپت**:
   - بعد از متوقف کردن پروسه موجود، اسکریپت رو دوباره اجرا کنید:
     ```bash
     python test.py
     ```
   - V2Root حالا باید بدون خطای -7 با موفقیت شروع بشه.

6. **چک کردن تداخل پورت‌ها**:
   - پروسه قبلی V2Ray ممکنه هنوز پورت‌های 2300 (HTTP) یا 2301 (SOCKS) رو گرفته باشه.
   - توی ویندوز:
     ```powershell
     netstat -an | findstr 2300
     netstat -an | findstr 2301
     ```
     - اگه پورت‌ها استفاده شدن، برنامه رو با Task Manager پیدا کنید و ببندید.
   - توی لینوکس:
     ```bash
     netstat -tuln | grep 2300
     netstat -tuln | grep 2301
     ```
     - اگه پورت‌ها مشغولن، پروسه رو پیدا کنید:
       ```bash
       sudo lsof -i :2300
       sudo kill -9 <PID>
       ```

7. **بررسی فایل لاگ**:
   - فایل `v2root.log` رو توی همون پوشه اسکریپتتون باز کنید.
   - دنبال ورودی‌های مربوط به مدیریت پروسه بگردید:
     - "V2Ray process already running with PID: ..." (PID رو یادداشت کنید و خاتمه بدید).
     - "Failed to stop V2Ray process" (یعنی پروسه گیر کرده).
   - لاگ رو ببینید:
     ```bash
     cat v2root.log
     ```

8. **ری‌استارت کردن کامپیوتر (آخرین راه)**:
   - اگه پروسه گیر کرده و خاتمه پیدا نمی‌کنه، کامپیوتر رو ری‌استارت کنید.
   - این کار همه پروسه‌ها رو اجباری می‌بنده و هر قفل رجیستری یا سیستمی رو پاک می‌کنه.

### چطور از این خطا جلوگیری کنیم؟
برای جلوگیری از خطای -7 در آینده:

1. **همیشه V2Ray رو قبل از شروع اتصال جدید متوقف کنید**:
   - یه دستور توقف قبل از شروع اضافه کنید:
     ```python
     v2root = V2ROOT()
     v2root.stop()  # هر پروسه موجود رو متوقف کنید
     v2root.set_config_string(config_str)
     v2root.start()  # با تنظیمات جدید شروع کنید
     ```

2. **از بلوک‌های Try-Finally استفاده کنید**:
   - اطمینان حاصل کنید V2Ray حتی اگه اسکریپت کرش کرد متوقف می‌شه:
     ```python
     v2root = V2ROOT()
     try:
         v2root.set_config_string(config_str)
         v2root.start()
         # کد شما اینجا
     finally:
         v2root.stop()  # همیشه موقع تمام شدن متوقف کنید
     ```

3. **چک کردن وضعیت پروسه قبل از شروع**:
   - تأیید کنید هیچ پروسه V2Ray در حال اجرا نیست قبل از شروع یکی جدید:
     ```python
     import subprocess
     
     # چک کردن پروسه موجود
     result = subprocess.run(['tasklist'], capture_output=True, text=True)  # ویندوز
     # result = subprocess.run(['ps', 'aux'], capture_output=True, text=True)  # لینوکس
     
     if 'v2ray' in result.stdout.lower():
         print("V2Ray در حال اجراست. اول متوقفش می‌کنیم...")
         v2root.stop()
     ```

4. **چندین اسکریپت رو همزمان اجرا نکنید**:
   - از اجرای چندین اسکریپت پایتون که از V2Root استفاده می‌کنن به‌طور همزمان خودداری کنید.
   - اگه نیاز دارید چندین تنظیمات رو تست کنید، V2Ray رو بین تست‌ها متوقف کنید.

### هنوز مشکل دارید؟
اگه همه مراحل رو امتحان کردید و هنوز مشکل دارید، ما اینجاییم که کمک کنیم! با این اطلاعات با ما تماس بگیرید:
- فایل اسکریپتی که اجرا می‌کنید (مثل `test.py`).
- فایل `v2root.log` توی همون پوشه.
- نوع سیستم عاملتون (ویندوز یا لینوکس).
- توضیحی از اینکه خطا کی شروع شد (مثلا "بعد از کرش" یا "موقع اجرای چندین اسکریپت").
- مشکل رو گزارش کنید توی:
  - تلگرام: @Sepehr0Day
  - گیت‌هاب: https://github.com/V2RayRoot/V2Root/issues
