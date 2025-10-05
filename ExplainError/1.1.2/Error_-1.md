# Error Code -1: General Error

## English

### What is Error Code -1?
Error Code -1 is a "General Error" in V2Root, meaning something unexpected prevented the program from completing your request. This is a catch-all error for issues like missing files, insufficient permissions, or system misconfigurations when processing your configuration string (e.g., `vless://`, `vmess://`, `ss://`).

### Why Does This Happen?
This error can occur due to:
- The configuration string is invalid or cannot be processed to create `config.json`.
- On Linux, V2Ray is not installed or is outdated.
- The program lacks permissions to access files or network resources.
- The V2Root library file (`libv2root.dll` on Windows, `libv2root.so` on Linux) is missing or inaccessible.
- Conflicts with antivirus or firewall software.

### How to Fix It
Follow these steps carefully to resolve the issue:

1. **Validate the Configuration String**:
   - Ensure your configuration string starts with a supported protocol:
     - `vless://`
     - `vmess://`
     - `ss://`
   - Example valid string:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - Check for typos in the user ID, server address, or port. Contact your VPN provider for a correct string if unsure.

2. **Verify V2Ray Installation (Linux Only)**:
   - Open a terminal and check if V2Ray is installed:
     ```bash
     v2ray version
     ```
   - If you see a version number (e.g., `V2Ray 5.12.1`), V2Ray is installed. If it’s outdated or missing, install the latest version manually.
   - Follow the instructions in <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a>.

3. **Check V2Root Library File**:
   - On Windows, ensure `libv2root.dll` exists in the V2Root folder, for example:
     ```
     C:\V2Root\libv2root.dll
     ```
     Replace `C:\V2Root\` with the actual folder where you extracted V2Root.
     - Check if it exists:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - If missing, contact support (see below).
   - On Linux, ensure `libv2root.so` exists, for example:
     ```
     /usr/local/lib/v2root/libv2root.so
     ```
     Replace `/usr/local/lib/v2root/` with the actual V2Root folder path.
     - Check if it exists:
       ```bash
       ls /usr/local/lib/v2root/libv2root.so
       ```
     - Ensure it’s readable:
       ```bash
       chmod +r /usr/local/lib/v2root/libv2root.so
       ```
     - If missing, contact support.

4. **Run as Administrator**:
   - On Windows:
     - Right-click `test.py` and select "Run as administrator".
     - Or open PowerShell as admin and run:
       ```powershell
       python test.py
       ```
   - On Linux:
     - Use `sudo` to run the script:
       ```bash
       sudo python3 test.py
       ```
     - If you get a permission error, ensure the script is executable:
       ```bash
       chmod +x test.py
       ```

5. **Inspect the Log File**:
   - Open `v2root.log` in the same folder as `test.py` using a text editor.
   - Look for errors like:
     - "Invalid configuration string" (check your configuration string).
     - "File not found" (check `libv2root.dll` or `libv2root.so`).
     - "Permission denied" (run as admin or fix permissions).
     - "V2Ray core not found" (Linux: install V2Ray per <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a>).
   - On Linux, view the log:
     ```bash
     cat v2root.log
     ```

6. **Check for Software Conflicts**:
   - Ensure no antivirus or firewall blocks V2Root.
   - On Windows, add exceptions for `test.py` and `C:\V2Root\libv2root.dll` (replace with your path) in Windows Defender:
     - Settings > Update & Security > Windows Security > Virus & Threat Protection > Manage Settings > Exclusions > Add an exclusion.
   - On Linux, check if `ufw` blocks ports 2300 or 2301:
     ```bash
     sudo ufw status
     ```
     Allow them:
     ```bash
     sudo ufw allow 2300
     sudo ufw allow 2301
     ```

7. **Verify System Requirements**:
   - Ensure your system meets V2Root’s requirements:
     - Windows 7 or later (64-bit recommended).
     - Linux: Ubuntu 18.04+, CentOS 7+, Arch, Fedora, or compatible.
     - At least 2GB RAM and 500MB free disk space.
   - Update your system:
     - Windows: Run Windows Update.
     - Linux:
       ```bash
       sudo apt update && sudo apt upgrade
       ```

### Still Stuck?
If you’ve tried all the steps and the issue persists, we’re here to help! Contact us with the following details:
- The script file you’re running (e.g., `test.py`).
- The configuration string you’re using (remove sensitive parts like user ID).
- The `v2root.log` file from the same folder.
- Your operating system (Windows or Linux).
- Report the issue on:
  - Telegram: @Sepehr0Day
  - GitHub: https://github.com/V2RayRoot/V2Root/issues

---

## Persian (فارسی)

### خطای کد -۱ چیست؟
خطای کد -۱ یه "خطای عمومی" توی V2Rootه، یعنی یه مشکل غیرمنتظره باعث شده برنامه نتونه درخواست شما رو انجام بده. این خطا برای مشکلاتی مثل فایل‌های گمشده، کمبود دسترسی یا تنظیمات اشتباه سیستم موقع پردازش رشته تنظیمات (مثل `vless://`، `vmess://`، `ss://`) پیش میاد.

### چرا این خطا رخ می‌ده؟
دلایلش می‌تونه اینا باشه:
- رشته تنظیمات اشتباهه یا نمی‌تونه برای ساخت `config.json` پردازش بشه.
- توی لینوکس، V2Ray نصب نیست یا قدیمی شده.
- برنامه دسترسی لازم برای خوندن فایل‌ها یا شبکه رو نداره.
- فایل کتابخونه V2Root (`libv2root.dll` توی ویندوز، `libv2root.so` توی لینوکس) گم شده یا قابل دسترسی نیست.
- تداخل با آنتی‌ویروس یا فایروال.

### چطور درستش کنیم؟
این مراحل رو با دقت دنبال کنید تا مشکل حل بشه:

1. **بررسی رشته تنظیمات**:
   - مطمئن بشید رشته تنظیمات با یه پروتکل پشتیبانی‌شده شروع می‌شه:
     - `vless://`
     - `vmess://`
     - `ss://`
   - نمونه رشته درست:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - آیدی کاربر، آدرس سرور و پورت رو برای اشتباه تایپی چک کنید. اگه مطمئن نیستید، از ارائه‌دهنده VPN رشته درست رو بگیرید.

2. **بررسی نصب V2Ray (فقط لینوکس)**:
   - ترمینال رو باز کنید و چک کنید V2Ray نصب شده:
     ```bash
     v2ray version
     ```
   - اگه نسخه‌ای دیدید (مثل `V2Ray 5.12.1`)، نصبه. اگه قدیمی یا غایبه، آخرین نسخه رو دستی نصب کنید.
   - راهنمای نصب رو توی <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a> دنبال کنید.

3. **چک کردن فایل کتابخونه V2Root**:
   - توی ویندوز، مطمئن بشید `libv2root.dll` توی پوشه V2Root هست، مثلا:
     ```
     C:\V2Root\libv2root.dll
     ```
     `C:\V2Root\` رو با مسیر واقعی پوشه V2Root عوض کنید.
     - چک کنید فایل هست:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید (پایین رو ببینید).
   - توی لینوکس، مطمئن بشید `libv2root.so` هست، مثلا:
     ```
     /usr/local/lib/v2root/libv2root.so
     ```
     `/usr/local/lib/v2root/` رو با مسیر واقعی V2Root عوض کنید.
     - چک کنید فایل هست:
       ```bash
       ls /usr/local/lib/v2root/libv2root.so
       ```
     - دسترسیش رو درست کنید:
       ```bash
       chmod +r /usr/local/lib/v2root/libv2root.so
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید.

4. **اجرا با دسترسی مدیر**:
   - توی ویندوز:
     - روی `test.py` راست‌کلیک کنید و "Run as administrator" رو انتخاب کنید.
     - یا پاورشل رو به‌عنوان مدیر باز کنید و بنویسید:
       ```powershell
       python test.py
       ```
   - توی لینوکس:
     - با `sudo` اسکریپت رو اجرا کنید:
       ```bash
       sudo python3 test.py
       ```
     - اگه خطای دسترسی داد، فایل رو قابل اجرا کنید:
       ```bash
       chmod +x test.py
       ```

5. **بررسی فایل لاگ**:
   - فایل `v2root.log` توی همون پوشه `test.py` رو با ویرایشگر متن باز کنید.
   - دنبال خطاها بگردید، مثل:
     - "Invalid configuration string" (رشته تنظیمات رو چک کنید).
     - "File not found" (`libv2root.dll` یا `libv2root.so` رو چک کنید).
     - "Permission denied" (با دسترسی مدیر اجرا کنید یا دسترسی رو درست کنید).
     - "V2Ray core not found" (لینوکس: V2Ray رو طبق <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a> نصب کنید).
   - توی لینوکس لاگ رو ببینید:
     ```bash
     cat v2root.log
     ```

6. **چک کردن تداخل نرم‌افزاری**:
   - مطمئن بشید آنتی‌ویروس یا فایروال V2Root رو بلاک نکرده.
   - توی ویندوز، برای `test.py` و `C:\V2Root\libv2root.dll` (مسیر واقعی رو بذارید) توی Windows Defender استثنا اضافه کنید:
     - تنظیمات > آپدیت و امنیت > امنیت ویندوز > محافظت از ویروس و تهدید > مدیریت تنظیمات > استثناها > اضافه کردن استثنا.
   - توی لینوکس، چک کنید `ufw` پورت‌های 2300 یا 2301 رو بلاک نکرده:
     ```bash
     sudo ufw status
     ```
     پورت‌ها رو باز کنید:
     ```bash
     sudo ufw allow 2300
     sudo ufw allow 2301
     ```

7. **بررسی نیازهای سیستمی**:
   - مطمئن بشید سیستم شما با نیازهای V2Root سازگاره:
     - ویندوز ۷ یا جدیدتر (۶۴ بیتی بهتره).
     - لینوکس: اوبونتو 18.04 به بالا، سنت‌اواس ۷ به بالا، آرچ، فدورا یا مشابه.
     - حداقل ۲ گیگ رم و ۵۰۰ مگ فضای خالی.
   - سیستم رو به‌روز کنید:
     - ویندوز: Windows Update رو اجرا کنید.
     - لینوکس:
       ```bash
       sudo apt update && sudo apt upgrade
       ```

### هنوز مشکل دارید؟
اگه همه مراحل رو امتحان کردید و هنوز مشکل دارید، ما اینجاییم که کمک کنیم! با این اطلاعات با ما تماس بگیرید:
- فایل اسکریپتی که اجرا می‌کنید (مثل `test.py`).
- رشته تنظیماتی که استفاده می‌کنید (بخش‌های حساس مثل آیدی کاربر رو حذف کنید).
- فایل `v2root.log` توی همون پوشه.
- نوع سیستم عاملتون (ویندوز یا لینوکس).
- مشکل رو گزارش کنید توی:
  - تلگرام: @Sepehr0Day
  - گیت‌هاب: https://github.com/V2RayRoot/V2Root/issues