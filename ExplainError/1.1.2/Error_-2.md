# Error Code -2: Service Error

## English

### What is Error Code -2?
Error Code -2 is a "Service Error" in V2Root, indicating that the V2Ray program failed to start or couldn’t connect to the internet after processing your configuration string. This error relates to issues with launching the V2Ray core or establishing network connectivity.

### Why Does This Happen?
Common causes include:
- On Linux, V2Ray is not installed or is outdated, preventing the service from starting.
- Network ports (e.g., 2300, 2301) are already in use by another program.
- A firewall or antivirus is blocking V2Ray’s network access.
- Internet connectivity issues (e.g., no internet or server downtime).
- The configuration string caused an invalid `config.json` to be generated.

### How to Fix It
Follow these detailed steps to resolve the issue:

1. **Validate the Configuration String**:
   - Ensure your configuration string starts with:
     - `vless://`
     - `vmess://`
     - `ss://`
   - Example valid string:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - Check for typos in the user ID, server address, or port. Contact your VPN provider for a correct string.

2. **Verify V2Ray Installation (Linux Only)**:
   - Check if V2Ray is installed:
     ```bash
     v2ray --version
     ```
   - If missing or outdated, install the latest version manually per <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a>.

3. **Check V2Root Library File**:
   - On Windows, ensure `libv2root.dll` exists, e.g.:
     ```
     C:\V2Root\libv2root.dll
     ```
     Replace `C:\V2Root\` with your V2Root folder path.
     - Check:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - If missing, contact support.
   - On Linux, ensure `libv2root.so` exists, e.g.:
     ```
     /usr/local/lib/v2root/libv2root.so
     ```
     Replace `/usr/local/lib/v2root/` with your path.
     - Check:
       ```bash
       ls /usr/local/lib/v2root/libv2root.so
       ```
     - Ensure readable:
       ```bash
       chmod +r /usr/local/lib/v2root/libv2root.so
       ```
     - If missing, contact support.

4. **Check Port Availability**:
   - Ensure ports 2300 (HTTP) and 2301 (SOCKS) are free:
     - On Linux:
       ```bash
       netstat -tuln | grep 2300
       netstat -tuln | grep 2301
       ```
       If in use, find the program:
       ```bash
       sudo lsof -i :2300
       ```
       Stop it or change ports in your script.
     - On Windows:
       ```powershell
       netstat -an | findstr 2300
       netstat -an | findstr 2301
       ```
       If used, close the program via Task Manager or change ports.

5. **Test Internet Connectivity**:
   - Check internet:
     ```bash
     ping 8.8.8.8
     ```
   - If no response, restart your router or contact your ISP.
   - Test the server in your configuration string:
     ```bash
     ping server-address
     ```
     Replace `server-address` with the address from your string. If it fails, contact your VPN provider.

6. **Check Firewall and Antivirus**:
   - On Windows, allow `test.py` and `C:\V2Root\libv2root.dll` (use your path) in Windows Defender:
     - Settings > Update & Security > Windows Security > Virus & Threat Protection > Manage Settings > Exclusions.
   - On Linux, allow ports:
     ```bash
     sudo ufw allow 2300
     sudo ufw allow 2301
     sudo ufw status
     ```

7. **Inspect the Log File**:
   - Open `v2root.log` and look for:
     - "Port already in use" (free ports).
     - "Network unreachable" (check internet/server).
     - "Invalid config" (check configuration string).
   - View log:
     ```bash
     cat v2root.log
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

### خطای کد -۲ چیست؟
خطای کد -۲ یه "خطای سرویس" توی V2Rootه، یعنی V2Ray نتونسته بعد از پردازش رشته تنظیمات شروع کنه یا به اینترنت وصل بشه. این خطا به مشکلات راه‌اندازی هسته V2Ray یا اتصال شبکه مربوط می‌شه.

### چرا این خطا رخ می‌ده؟
دلایلش می‌تونه اینا باشه:
- توی لینوکس، V2Ray نصب نیست یا قدیمیه.
- پورت‌های شبکه (مثل 2300، 2301) توسط برنامه دیگه‌ای استفاده شدن.
- فایروال یا آنتی‌ویروس دسترسی شبکه V2Ray رو بلاک کرده.
- مشکل اتصال اینترنت (مثل قطعی یا خاموش بودن سرور).
- رشته تنظیمات باعث شده `config.json` نادرست ساخته بشه.

### چطور درستش کنیم؟
این مراحل رو دنبال کنید:

1. **بررسی رشته تنظیمات**:
   - مطمئن بشید رشته با اینا شروع می‌شه:
     - `vless://`
     - `vmess://`
     - `ss://`
   - نمونه:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - آیدی، آدرس سرور و پورت رو چک کنید. از ارائه‌دهنده VPN رشته درست رو بگیرید.

2. **بررسی نصب V2Ray (فقط لینوکس)**:
   - چک کنید V2Ray نصب شده:
     ```bash
     v2ray --version
     ```
   - اگه نیست یا قدیمیه، آخرین نسخه رو طبق <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a> نصب کنید.

3. **چک کردن فایل کتابخونه V2Root**:
   - توی ویندوز، مطمئن بشید `libv2root.dll` هست، مثلا:
     ```
     C:\V2Root\libv2root.dll
     ```
     `C:\V2Root\` رو با مسیر خودتون عوض کنید.
     - چک کنید:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید.
   - توی لینوکس، چک کنید `libv2root.so` هست، مثلا:
     ```
     /usr/local/lib/v2root/libv2root.so
     ```
     مسیر رو عوض کنید.
     - چک کنید:
       ```bash
       ls /usr/local/lib/v2root/libv2root.so
       ```
     - دسترسی بدید:
       ```bash
       chmod +r /usr/local/lib/v2root/libv2root.so
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید.

4. **چک کردن پورت‌ها**:
   - مطمئن بشید پورت‌های 2300 (HTTP) و 2301 (SOCKS) آزادن:
     - لینوکس:
       ```bash
       netstat -tuln | grep 2300
       netstat -tuln | grep 2301
       ```
       اگه استفاده شدن:
       ```bash
       sudo lsof -i :2300
       ```
       برنامه رو ببندید یا پورت رو عوض کنید.
     - ویندوز:
       ```powershell
       netstat -an | findstr 2300
       netstat -an | findstr 2301
       ```
       برنامه رو توی Task Manager ببندید یا پورت عوض کنید.

5. **تست اتصال اینترنت**:
   - اینترنت رو چک کنید:
     ```bash
     ping 8.8.8.8
     ```
   - اگه کار نکرد، روتر رو ری‌استارت کنید یا با ISP تماس بگیرید.
   - سرور رشته تنظیمات رو تست کنید:
     ```bash
     ping server-address
     ```
     `server-address` رو از رشته بردارید. اگه کار نکرد، با ارائه‌دهنده VPN تماس بگیرید.

6. **بررسی فایروال و آنتی‌ویروس**:
   - توی ویندوز، برای `test.py` و `C:\V2Root\libv2root.dll` (مسیر خودتون) استثنا اضافه کنید:
     - تنظیمات > آپدیت و امنیت > امنیت ویندوز > محافظت از ویروس و تهدید > مدیریت تنظیمات > استثناها.
   - توی لینوکس:
     ```bash
     sudo ufw allow 2300
     sudo ufw allow 2301
     sudo ufw status
     ```

7. **بررسی فایل لاگ**:
   - `v2root.log` رو باز کنید و دنبال اینا بگردید:
     - "Port already in use" (پورت‌ها رو آزاد کنید).
     - "Network unreachable" (اینترنت/سرور رو چک کنید).
     - "Invalid config" (رشته تنظیمات رو چک کنید).
   - لاگ:
     ```bash
     cat v2root.log
     ```

### هنوز مشکل دارید؟
با این اطلاعات تماس بگیرید:
- فایل اسکریپت (مثل `test.py`).
- رشته تنظیمات (بدون آیدی کاربر).
- فایل `v2root.log`.
- سیستم عامل (ویندوز یا لینوکس).
- گزارش مشکل:
  - تلگرام: @Sepehr0Day
  - گیت‌هاب: https://github.com/V2RayRoot/V2Root/issues