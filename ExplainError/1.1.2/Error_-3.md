# Error Code -3: Config Error

## English

### What is Error Code -3?
Error Code -3 is a "Config Error" in V2Root, meaning the V2Ray configuration string you provided (e.g., `vless://`, `vmess://`, `ss://`) is incorrect, malformed, or incompatible. This prevents V2Root from generating a valid `config.json` file, which stops V2Ray from starting or connecting properly.

### Why Does This Happen?
This error can occur because:
- The configuration string has an invalid format or missing components (e.g., wrong protocol, missing user ID, or incorrect server address).
- The server address, port, or user ID in the configuration string is wrong or outdated.
- The V2Ray protocol in the string (e.g., `vless`, `vmess`) is not supported by your version of V2Ray or V2Root.
- The program failed to parse the configuration string due to a bug or unsupported characters.
- The VPN provider gave you an incorrect or expired configuration string.

### How to Fix It
Follow these detailed steps to resolve the issue:

1. **Validate the Configuration String**:
   - Ensure your configuration string starts with a supported protocol:
     - `vless://`
     - `vmess://`
     - `ss://`
   - Example of a valid string:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - Check for:
     - **User ID**: A valid UUID or identifier (e.g., `123e4567-e89b-12d3-a456-426614174000`).
     - **Server Address**: A correct IP address (e.g., `192.168.1.1`) or domain (e.g., `vpn.example.com`).
     - **Port**: A number like `443` or `1080`.
     - **Parameters**: Correct settings like `security=tls` or `type=tcp`.
   - Compare your string with examples from your VPN provider. Fix any typos or missing parts.

2. **Contact Your VPN Provider**:
   - If you’re unsure about the configuration string, send it to your VPN provider (remove sensitive parts like the user ID if needed) and ask them to verify:
     - Is the protocol (`vless`, `vmess`, `ss`) correct?
     - Is the server address and port still active?
     - Are the user ID and other parameters valid?
   - Request a new configuration string if yours is outdated or incorrect.

3. **Test the Server Address**:
   - Extract the server address from your configuration string (e.g., `server-address` in `vless://user-id@server-address:443`).
   - Test if it’s reachable:
     ```bash
     ping server-address
     ```
   - If the ping fails, the server may be down or the address is wrong. Ask your VPN provider for an alternative server.

4. **Test a Different Configuration String**:
   - Ask your VPN provider for another configuration string (e.g., a different server or protocol).
   - Update your script to use the new string (e.g., pass it to `set_config_string`) and rerun `test.py`.

5. **Update V2Ray (Linux Only)**:
   - An outdated V2Ray version may not support newer protocols in the configuration string. Check the version:
     ```bash
     v2ray --version
     ```
   - If it’s missing or old, update or install V2Ray:
     - For Ubuntu/Debian:
       ```bash
       sudo apt update
       sudo apt install v2ray
       ```
     - For CentOS/RHEL:
       ```bash
       sudo yum install v2ray
       ```
     - For other distros, download from https://github.com/v2fly/v2ray-core/releases and follow the installation guide.

6. **Check Windows Setup**:
   - On Windows, V2Ray is bundled with V2Root, so no separate installation is needed.
   - Ensure the file `libv2root.dll` is in the same folder as `test.py` or in the `lib/build_win` subdirectory.
   - If missing, redownload V2Root from https://github.com/V2RayRoot/V2Root/releases.

7. **Inspect the Log File**:
   - Open `v2root.log` in the same folder as `test.py` with a text editor (e.g., Notepad on Windows, `nano` on Linux).
   - Look for errors related to the configuration string or `config.json` generation, such as:
     - "Invalid configuration string" (check string format).
     - "Failed to parse JSON" (indicates V2Root couldn’t create a valid `config.json` from the string).
     - "Unknown protocol" (use a supported protocol like `vless` or `vmess`).
     - "Server rejected" (wrong user ID, server address, or port).
   - View the log on Linux:
     ```bash
     cat v2root.log
     ```
   - If the log mentions `config.json`, it may show the generated file’s contents. Check for errors like missing fields or invalid JSON syntax.

8. **Reinstall V2Root**:
   - If the program is failing to process the configuration string, there might be a bug or corrupted files.
   - Delete the V2Root folder and redownload the latest version from https://github.com/V2RayRoot/V2Root/releases.
   - Extract and rerun `test.py` with your configuration string.

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

### خطای کد -۳ چیست؟
خطای کد -۳ یه "خطای تنظیمات" توی V2Rootه، یعنی رشته تنظیمات V2Ray که دادید (مثل `vless://`، `vmess://`، `ss://`) اشتباه، ناقص یا ناسازگاره. این باعث می‌شه V2Root نتونه فایل `config.json` درست رو بسازه و V2Ray شروع یا وصل نشه.

### چرا این خطا رخ می‌ده؟
دلایلش می‌تونه اینا باشه:
- رشته تنظیمات فرمت اشتباه یا قطعات گمشده داره (مثل پروتکل نادرست، آیدی گمشده یا آدرس سرور اشتباه).
- آدرس سرور، پورت یا آیدی کاربر توی رشته تنظیمات اشتباه یا قدیمی شده.
- پروتکل V2Ray توی رشته (مثل `vless`، `vmess`) توسط نسخه V2Ray یا V2Root شما پشتیبانی نمی‌شه.
- برنامه نتونسته رشته تنظیمات رو به خاطر باگ یا کاراکترهای ناسازگار پردازش کنه.
- ارائه‌دهنده VPN یه رشته تنظیمات اشتباه یا منقضی شده داده.

### چطور درستش کنیم؟
این مراحل رو با دقت دنبال کنید تا مشکل حل بشه:

1. **بررسی رشته تنظیمات**:
   - مطمئن بشید رشته تنظیمات با یه پروتکل پشتیبانی‌شده شروع می‌شه:
     - `vless://`
     - `vmess://`
     - `ss://`
   - نمونه یه رشته درست:
     ```
     vless://user-id@server-address:443?security=tls&type=tcp#MyVPN
     ```
   - اینا رو چک کنید:
     - **آیدی کاربر**: یه UUID یا شناسه معتبر (مثل `123e4567-e89b-12d3-a456-426614174000`).
     - **آدرس سرور**: یه IP درست (مثل `192.168.1.1`) یا دامنه (مثل `vpn.example.com`).
     - **پورت**: یه شماره مثل `443` یا `1080`.
     - **پارامترها**: تنظیمات درست مثل `security=tls` یا `type=tcp`.
   - رشته‌تون رو با نمونه‌های ارائه‌دهنده VPN مقایسه کنید. خطاهای تایپی یا قطعات گمشده رو درست کنید.

2. **تماس با ارائه‌دهنده VPN**:
   - اگه از رشته تنظیمات مطمئن نیستید، اون رو (بدون بخش‌های حساس مثل آیدی کاربر) برای ارائه‌دهنده VPN بفرستید و بخواید چک کنه:
     - پروتکل (`vless`، `vmess`، `ss`) درسته؟
     - آدرس سرور و پورت هنوز فعاله؟
     - آیدی کاربر و بقیه پارامترها معتبرن؟
   - اگه رشته قدیمی یا اشتباهه، یه رشته تنظیمات جدید بخواید.

3. **تست آدرس سرور**:
   - آدرس سرور رو از رشته تنظیمات بردارید (مثل `server-address` توی `vless://user-id@server-address:443`).
   - چک کنید قابل دسترسیه:
     ```bash
     ping server-address
     ```
   - اگه پینگ کار نکرد، سرور شاید خاموشه یا آدرس اشتباهه. از ارائه‌دهنده VPN یه سرور دیگه بخواید.

4. **امتحان یه رشته تنظیمات دیگه**:
   - از ارائه‌دهنده VPN یه رشته تنظیمات دیگه (مثل سرور یا پروتکل متفاوت) بخواید.
   - اسکریپتتون رو آپدیت کنید تا از رشته جدید استفاده کنه (مثلا به `set_config_string` بدید) و `test.py` رو دوباره اجرا کنید.

5. **به‌روز کردن V2Ray (فقط لینوکس)**:
   - نسخه قدیمی V2Ray ممکنه پروتکل‌های جدید توی رشته تنظیمات رو پشتیبانی نکنه. نسخه رو چک کنید:
     ```bash
     v2ray --version
     ```
   - اگه نیست یا قدیمیه، V2Ray رو نصب یا آپدیت کنید:
     - برای اوبونتو/دبیان:
       ```bash
       sudo apt update
       sudo apt install v2ray
       ```
     - برای سنت‌اواس/رد هت:
       ```bash
       sudo yum install v2ray
       ```
     - برای توزیع‌های دیگه، از https://github.com/v2fly/v2ray-core/releases دانلود کنید و راهنمای نصب رو دنبال کنید.

6. **بررسی تنظیمات ویندوز**:
   - توی ویندوز، V2Ray همراه V2Rootه، پس نیازی به نصب جداگونه نیست.
   - مطمئن بشید فایل `libv2root.dll` توی همون پوشه `test.py` یا توی زیرپوشه `lib/build_win` هست.
   - اگه نیست، V2Root رو از https://github.com/V2RayRoot/V2Root/releases دوباره دانلود کنید.

7. **بررسی فایل لاگ**:
   - فایل `v2root.log` توی پوشه `test.py` رو با یه ویرایشگر متن (مثل نوت‌پد توی ویندوز یا `nano` توی لینوکس) باز کنید.
   - دنبال خطاهای مربوط به رشته تنظیمات یا ساخت `config.json` بگردید، مثل:
     - "Invalid configuration string" (فرمت رشته رو چک کنید).
     - "Failed to parse JSON" (یعنی V2Root نتونسته از رشته یه `config.json` درست بسازه).
     - "Unknown protocol" (از پروتکل‌های پشتیبانی‌شده مثل `vless` یا `vmess` استفاده کنید).
     - "Server rejected" (آیدی کاربر، آدرس سرور یا پورت اشتباهه).
   - توی لینوکس لاگ رو ببینید:
     ```bash
     cat v2root.log
     ```
   - اگه لاگ به `config.json` اشاره کرد، ممکنه محتوای فایل ساخته‌شده رو نشون بده. خطاهایی مثل فیلدهای گمشده یا نحو JSON اشتباه رو چک کنید.

8. **نصب دوباره V2Root**:
   - اگه برنامه نمی‌تونه رشته تنظیمات رو پردازش کنه، شاید باگ یا فایل‌های خراب باشه.
   - پوشه V2Root رو حذف کنید و آخرین نسخه رو از https://github.com/V2RayRoot/V2Root/releases دانلود کنید.
   - فایل‌ها رو استخراج کنید و `test.py` رو با رشته تنظیماتتون دوباره اجرا کنید.

### هنوز مشکل دارید؟
اگه همه مراحل رو امتحان کردید و هنوز مشکل دارید، ما اینجاییم که کمک کنیم! با این اطلاعات با ما تماس بگیرید:
- فایل اسکریپتی که اجرا می‌کنید (مثل `test.py`).
- رشته تنظیماتی که استفاده می‌کنید (بخش‌های حساس مثل آیدی کاربر رو حذف کنید).
- فایل `v2root.log` توی همون پوشه.
- نوع سیستم عاملتون (ویندوز یا لینوکس).
- مشکل رو گزارش کنید توی:
  - تلگرام: @Sepehr0Day
  - گیت‌هاب: https://github.com/V2RayRoot/V2Root/issues