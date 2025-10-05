# Error Code -6: Proxy Error

## English

### What is Error Code -6?
Error Code -6 is a "Proxy Error" in V2Root, meaning the program couldn’t set or clear your system’s proxy settings after processing your configuration string. This affects V2Root’s ability to configure V2Ray as a proxy.

### Why Does This Happen?
Causes include:
- Insufficient permissions to modify proxy settings.
- Other VPN/proxy software interfering.
- Corrupted or locked system proxy settings.
- Antivirus blocking proxy changes.
- Issues with the generated `config.json` from the configuration string.

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
   - Check typos. Contact your VPN provider for a valid string.

2. **Run as Administrator**:
   - Windows: Right-click `test.py` > "Run as administrator".
     - Or PowerShell as admin:
       ```powershell
       python test.py
       ```
   - Linux:
     ```bash
     sudo python3 test.py
     ```
     - Ensure executable:
       ```bash
       chmod +x test.py
       ```

3. **Check V2Root Library File**:
   - Windows: Ensure `libv2root.dll`, e.g.:
     ```
     C:\V2Root\libv2root.dll
     ```
     Replace `C:\V2Root\`.
     - Check:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - If missing, contact support.
   - Linux: Ensure `libv2root.so`, e.g.:
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

4. **Check for Conflicting Software**:
   - Close other VPNs/proxies.
   - Windows: Settings > Network & Internet > Proxy > No manual proxy unless needed.
   - Linux:
     ```bash
     sudo systemctl stop openvpn
     ```

5. **Reset Proxy Settings**:
   - Windows:
     ```powershell
     netsh winhttp reset proxy
     ```
     Restart PC.
   - Linux:
     ```bash
     gsettings reset org.gnome.system.proxy
     ```

6. **Check Antivirus**:
   - Windows: Add exceptions for `test.py` and `C:\V2Root\libv2root.dll` (your path) in Windows Defender.
   - Linux: Ensure no security software blocks V2Root.

7. **Inspect the Log File**:
   - Open `v2root.log` for:
     - "Permission denied" (run as admin).
     - "Proxy setting failed" (check conflicts).
     - "Invalid config" (check configuration string).
   - View:
     ```bash
     cat v2root.log
     ```

### Still Stuck?
Contact with:
- Script (e.g., `test.py`).
- Configuration string (no sensitive parts).
- `v2root.log`.
- OS.
- Report:
  - Telegram: @Sepehr0Day
  - GitHub: https://github.com/V2RayRoot/V2Root/issues

---

## Persian (فارسی)

### خطای کد -۶ چیست؟
خطای کد -۶ یه "خطای پراکسی" توی V2Rootه، یعنی برنامه نتونسته تنظیمات پراکسی سیستم رو بعد از پردازش رشته تنظیمات درست کنه یا پاک کنه.

### چرا این خطا رخ می‌ده؟
دلایل:
- دسترسی ناکافی برای تغییر پراکسی.
- تداخل VPN/پراکسی دیگه.
- تنظیمات پراکسی خراب یا قفل.
- آنتی‌ویروس جلوی تغییر پراکسی رو گرفته.
- مشکل توی `config.json` ساخته‌شده از رشته تنظیمات.

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
   - خطاها رو چک کنید. از ارائه‌دهنده VPN رشته درست بگیرید.

2. **اجرا با دسترسی مدیر**:
   - ویندوز: روی `test.py` راست‌کلیک > "Run as administrator".
     - یا پاورشل به‌عنوان مدیر:
       ```powershell
       python test.py
       ```
   - لینوکس:
     ```bash
     sudo python3 test.py
     ```
     - قابل اجرا کنید:
       ```bash
       chmod +x test.py
       ```

3. **چک کردن فایل کتابخونه V2Root**:
   - ویندوز: `libv2root.dll`، مثلا:
     ```
     C:\V2Root\libv2root.dll
     ```
     مسیر رو درست کنید.
     - چک:
       ```powershell
       dir C:\V2Root\libv2root.dll
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید.
   - لینوکس: `libv2root.so`، مثلا:
     ```
     /usr/local/lib/v2root/libv2root.so
     ```
     مسیر رو درست کنید.
     - چک:
       ```bash
       ls /usr/local/lib/v2root/libv2root.so
       ```
     - دسترسی:
       ```bash
       chmod +r /usr/local/lib/v2root/libv2root.so
       ```
     - اگه نیست، با پشتیبانی تماس بگیرید.

4. **چک کردن نرم‌افزارهای متداخل**:
   - VPN/پراکسی‌های دیگه رو ببندید.
   - ویندوز: تنظیمات > شبکه و اینترنت > پراکسی > پراکسی دستی خاموش مگه لازم باشه.
   - لینوکس:
     ```bash
     sudo systemctl stop openvpn
     ```

5. **ریست تنظیمات پراکسی**:
   - ویندوز:
     ```powershell
     netsh winhttp reset proxy
     ```
     ری‌استارت کنید.
   - لینوکس:
     ```bash
     gsettings reset org.gnome.system.proxy
     ```

6. **بررسی آنتی‌ویروس**:
   - ویندوز: برای `test.py` و `C:\V2Root\libv2root.dll` (مسیر خودتون) استثنا اضافه کنید.
   - لینوکس: مطمئن بشید نرم‌افزار امنیتی بلاک نمی‌کنه.

7. **بررسی فایل لاگ**:
   - `v2root.log` رو برای:
     - "Permission denied" (با دسترسی مدیر).
     - "Proxy setting failed" (تداخل‌ها).
     - "Invalid config" (رشته تنظیمات).
   - لاگ:
     ```bash
     cat v2root.log
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