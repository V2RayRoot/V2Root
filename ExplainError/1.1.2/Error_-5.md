# Error Code -5: Initialization Error

## English

### What is Error Code -5?
Error Code -5 is an "Initialization Error" in V2Root, meaning the program couldn’t start V2Ray due to missing or misconfigured components when processing your configuration string. This occurs during the setup phase.

### Why Does This Happen?
Possible causes include:
- On Linux, V2Ray is not installed system-wide.
- The V2Root library (`libv2root.dll` on Windows, `libv2root.so` on Linux) is missing or inaccessible.
- Missing system libraries (e.g., `libjansson` on Linux).
- Insufficient permissions for V2Root files.
- Incompatible system environment.

### How to Fix It
Follow these steps:

1. **Validate the Configuration String**:
   - Ensure it starts with:
     - `vless://`
     - `vmess://`
     - `ss://`
   - Example:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - Check for typos. Contact your VPN provider for a valid string.

2. **Verify V2Ray Installation (Linux Only)**:
   - Check:
     ```bash
     v2ray version
     ```
   - If missing or outdated, install the latest V2Ray per <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a>.

3. **Check V2Root Library File**:
   - Windows: Ensure `libv2root.dll` exists, e.g.:
     ```
     C:\V2Root\libv2root.dll
     ```
     Replace `C:\V2Root\`.
     - Check:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - If missing, contact support.
   - Linux: Ensure `libv2root.so` exists, e.g.:
     ```
     /usr/local/lib/v2root/libv2root.so
     ```
     Replace path.
     - Check:
       ```bash
       ls /usr/local/lib/v2root/libv2root.so
       ```
     - Ensure readable:
       ```bash
       chmod +r /usr/local/lib/v2root/libv2root.so
       ```
     - If missing, contact support.

4. **Check for Missing Libraries (Linux Only)**:
   - Verify dependencies for `libv2root.so`:
     ```bash
     ldd /usr/local/lib/v2root/libv2root.so
     ```
     Replace the path with your `libv2root.so` location.
   - Install missing libraries, e.g.:
     ```bash
     sudo apt install libjansson-dev libc6
     ```

5. **Verify File Permissions**:
   - Linux: Ensure `test.py` and `libv2root.so` are accessible:
     ```bash
     ls -l test.py
     ls -l /usr/local/lib/v2root/libv2root.so
     chmod +x test.py
     chmod +r /usr/local/lib/v2root/libv2root.so
     ```
     Use your `libv2root.so` path.
   - Windows: Ensure `test.py` and `C:\V2Root\libv2root.dll` aren’t blocked:
     - Right-click > Properties > Unblock (if visible).

6. **Inspect the Log File**:
   - Open `v2root.log` for errors like:
     - "V2Ray core not found" (install V2Ray per <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a>).
     - "Library not found" (check `libv2root.dll` or `libv2root.so`).
     - "Invalid config" (check configuration string).
   - View:
     ```bash
     cat v2root.log
     ```

7. **Update System**:
   - Windows: Run Windows Update.
   - Linux:
     ```bash
     sudo apt update && sudo apt upgrade
     ```

### Still Stuck?
Contact us with:
- Script file (e.g., `test.py`).
- Configuration string (remove sensitive parts).
- `v2root.log`.
- OS (Windows or Linux).
- Report:
  - Telegram: @Sepehr0Day
  - GitHub: https://github.com/V2RayRoot/V2Root/issues

---

## Persian (فارسی)

### خطای کد -۵ چیست؟
خطای کد -۵ یه "خطای راه‌اندازی" توی V2Rootه، یعنی برنامه نتونسته V2Ray رو به خاطر قطعات گمشده یا تنظیمات اشتباه موقع پردازش رشته تنظیمات شروع کنه.

### چرا این خطا رخ می‌ده؟
دلایل:
- توی لینوکس، V2Ray نصب نیست.
- کتابخونه V2Root (`libv2root.dll` توی ویندوز، `libv2root.so` توی لینوکس) گم شده یا قابل دسترسی نیست.
- کتابخونه‌های سیستمی (مثل `libjansson` توی لینوکس) غایبن.
- دسترسی ناکافی برای فایل‌های V2Root.
- سیستم ناسازگار.

### چطور درستش کنیم؟
مراحل:

1. **بررسی رشته تنظیمات**:
   - باید با اینا شروع بشه:
     - `vless://`
     - `vmess://`
     - `ss://`
   - نمونه:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - خطاهای تایپی رو چک کنید. از ارائه‌دهنده VPN رشته معتبر بگیرید.

2. **بررسی نصب V2Ray (فقط لینوکس)**:
   - چک:
     ```bash
     v2ray version
     ```
   - اگه نیست یا قدیمیه، طبق <a href="https://github.com/V2RayRoot/V2Root/blob/main/ExplainError/1.1.2/Install_V2Ray_Linux.md">Install_V2Ray_Linux.md</a> نصب کنید.

3. **چک کردن فایل کتابخونه V2Root**:
   - ویندوز: مطمئن بشید `libv2root.dll` هست، مثلا:
     ```
     C:\V2Root\libv2root.dll
     ```
     مسیر رو عوض کنید.
     - چک:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید.
   - لینوکس: چک کنید `libv2root.so` هست، مثلا:
     ```
     /usr/local/lib/v2root/libv2root.so
     ```
     مسیر رو عوض کنید.
     - چک:
       ```bash
       ls /usr/local/lib/v2root/libv2root.so
       ```
     - دسترسی:
       ```bash
       chmod +r /usr/local/lib/v2root/libv2root.so
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید.

4. **بررسی کتابخونه‌های گمشده (لینوکس)**:
   - وابستگی‌ها:
     ```bash
     ldd /usr/local/lib/v2root/libv2root.so
     ```
     مسیر `libv2root.so` رو درست کنید.
   - نصب کتابخونه‌ها:
     ```bash
     sudo apt install libjansson-dev libc6
     ```

5. **بررسی دسترسی فایل‌ها**:
   - لینوکس:
     ```bash
     ls -l test.py
     ls -l /usr/local/lib/v2root/libv2root.so
     chmod +x test.py
     chmod +r /usr/local/lib/v2root/libv2root.so
     ```
     مسیر `libv2root.so` رو درست کنید.
   - ویندوز: مطمئن بشید `test.py` و `C:\V2Root\libv2root.dll` بلاک نشدن:
     - راست‌کلیک > Properties > Unblock.

6. **بررسی فایل لاگ**:
   - `v2root.log` رو چک کنید برای:
     - "V2Ray core not found" (V2Ray رو نصب کنید).
     - "Library not found" (`libv2root.dll` یا `libv2root.so`).
     - "Invalid config" (رشته تنظیمات).
   - لاگ:
     ```bash
     cat v2root.log
     ```

7. **به‌روز کردن سیستم**:
   - ویندوز: Windows Update.
   - لینوکس:
     ```bash
     sudo apt update && sudo apt upgrade
     ```

### هنوز مشکل دارید؟
با اینا تماس بگیرید:
- اسکریپت (مثل `test.py`).
- رشته تنظیمات (بدون بخش حساس).
- `v2root.log`.
- سیستم عامل.
- گزارش:
  - تلگرام: @Sepehr0Day
  - گیت‌هاب: https://github.com/V2RayRoot/V2Root/issues