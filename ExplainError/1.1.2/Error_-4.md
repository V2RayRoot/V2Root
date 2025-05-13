# Error Code -4: Connection Error

## English

### What is Error Code -4?
Error Code -4 is a "Connection Error" in V2Root, meaning the program couldn’t connect to the V2Ray server specified in your configuration string or use the network. This error occurs after V2Root generates `config.json` from the string and tries to establish a connection, indicating network or server issues.

### Why Does This Happen?
Possible causes include:
- The server address or port in the configuration string is incorrect or outdated.
- The V2Ray server is down, unreachable, or rejecting connections.
- Network ports (e.g., 2300 for HTTP, 2301 for SOCKS) are blocked by a firewall, router, or ISP.
- No internet connection or an unstable network.
- Other VPN or proxy software is interfering with V2Root’s network access.
- The generated `config.json` has issues due to errors in the configuration string processing.

### How to Fix It
Follow these detailed steps to resolve the issue:

1. **Verify Server Address and Port in the Configuration String**:
   - Check your configuration string (e.g., `vless://user-id@server-address:443?security=tls&type=tcp#MyVPN`).
   - Ensure the server address (e.g., `server-address`) and port (e.g., `443`) are correct.
   - Test the server’s reachability:
     ```bash
     ping server-address
     ```
   - If the ping fails, the server may be down or the address is wrong. Contact your VPN provider to confirm the server details or get a new configuration string.

2. **Test a Different Configuration String**:
   - Ask your VPN provider for an alternative configuration string with a different server or port.
   - Update your script to use the new string (e.g., pass it to `set_config_string`) and rerun `test.py`.

3. **Check Internet Connectivity**:
   - Ensure your internet is working:
     ```bash
     ping 8.8.8.8
     ```
   - If there’s no response, troubleshoot your network:
     - Restart your router or modem.
     - Check Wi-Fi or Ethernet connection.
     - Contact your ISP if the issue persists.

4. **Verify Port Availability**:
   - V2Root uses ports 2300 (HTTP) and 2301 (SOCKS) by default for local connections. Ensure they’re not blocked or in use.
   - On Linux:
     ```bash
     netstat -tuln | grep 2300
     netstat -tuln | grep 2301
     ```
     If ports are in use, identify the program:
     ```bash
     sudo lsof -i :2300
     ```
     Stop the conflicting program or modify your script to use different ports (consult V2Root documentation).
   - On Windows:
     ```powershell
     netstat -an | findstr 2300
     netstat -an | findstr 2301
     ```
     If ports are used, find the program in Task Manager and close it, or change ports in your script.

5. **Check Firewall and Antivirus Settings**:
   - On Windows, ensure Windows Defender or other antivirus allows `test.py` and `libv2root.dll`:
     - Go to Settings > Update & Security > Windows Security > Virus & Threat Protection > Manage Settings > Exclusions > Add an exclusion for both files.
   - On Linux, check if `ufw` or another firewall is blocking ports:
     ```bash
     sudo ufw status
     ```
     Allow V2Root’s ports:
     ```bash
     sudo ufw allow 2300
     sudo ufw allow 2301
     ```
   - Check your router’s firewall settings. If behind NAT, ensure ports 2300 and 2301 are forwarded (consult your router’s manual).
   - If your ISP blocks VPN ports, ask your VPN provider for a configuration string using a different port (e.g., 443, which is less likely to be blocked).

6. **Inspect the Log File**:
   - Open `v2root.log` in the same folder as `test.py` with a text editor.
   - Look for errors related to the connection or `config.json` generation, such as:
     - "Connection refused" (wrong server address/port or server down).
     - "Network timeout" (internet issue or server unreachable).
     - "Failed to parse config" (indicates an issue with the generated `config.json` due to the configuration string).
   - View the log on Linux:
     ```bash
     cat v2root.log
     ```
   - If the log shows `config.json` errors, it may indicate the configuration string was malformed. Double-check the string with your VPN provider.

7. **Check for VPN/Proxy Conflicts**:
   - Ensure no other VPN or proxy software (e.g., OpenVPN, NordVPN) is running, as they may interfere with V2Root.
   - On Windows, disable other VPNs:
     - Settings > Network & Internet > VPN > Disconnect any active VPNs.
   - On Linux, stop other VPN services:
     ```bash
     sudo systemctl stop openvpn
     ```

8. **Update V2Root and V2Ray**:
   - On Linux, ensure V2Ray is up-to-date to handle the configuration string and network protocols:
     ```bash
     v2ray --version
     sudo apt update
     sudo apt install v2ray
     ```
   - On Windows, ensure you have the latest V2Root version:
     - Delete the V2Root folder and redownload from https://github.com/V2RayRoot/V2Root/releases.
     - Verify `libv2root.dll` is present in the same folder as `test.py` or in `lib/build_win`.

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

### خطای کد -۴ چیست؟
خطای کد -۴ یه "خطای اتصال" توی V2Rootه، یعنی برنامه نتونسته به سرور V2Ray که توی رشته تنظیمات مشخص کردید وصل بشه یا از شبکه استفاده کنه. این خطا بعد از این پیش میاد که V2Root از رشته تنظیمات فایل `config.json` رو می‌سازه و سعی می‌کنه اتصال برقرار کنه، و نشون‌دهنده مشکلات شبکه یا سروره.

### چرا این خطا رخ می‌ده؟
دلایلش می‌تونه اینا باشه:
- آدرس سرور یا پورت توی رشته تنظیمات اشتباه یا قدیمی شده.
- سرور V2Ray خاموشه، در دسترس نیست یا اتصال رو رد می‌کنه.
- پورت‌های شبکه (مثل 2300 برای HTTP، 2301 برای SOCKS) توسط فایروال، روتر یا ISP بلاک شدن.
- اینترنت قطع یا ناپایداره.
- نرم‌افزار VPN یا پراکسی دیگه‌ای با دسترسی شبکه V2Root تداخل داره.
- فایل `config.json` ساخته‌شده مشکل داره چون پردازش رشته تنظیمات درست انجام نشده.

### چطور درستش کنیم؟
این مراحل رو با دقت دنبال کنید تا مشکل حل بشه:

1. **بررسی آدرس و پورت سرور توی رشته تنظیمات**:
   - رشته تنظیماتتون (مثل `vless://user-id@server-address:443?security=tls&type=tcp#MyVPN`) رو چک کنید.
   - مطمئن بشید آدرس سرور (مثل `server-address`) و پورت (مثل `443`) درسته.
   - دسترسی به سرور رو تست کنید:
     ```bash
     ping server-address
     ```
   - اگه پینگ کار نکرد، سرور شاید خاموشه یا آدرس اشتباهه. با ارائه‌دهنده VPN تماس بگیرید تا جزئیات سرور رو تأیید کنه یا یه رشته تنظیمات جدید بده.

2. **امتحان یه رشته تنظیمات دیگه**:
   - از ارائه‌دهنده VPN یه رشته تنظیمات دیگه با سرور یا پورت متفاوت بخواید.
   - اسکریپتتون رو آپدیت کنید تا از رشته جدید استفاده کنه (مثلا به `set_config_string` بدید) و `test.py` رو دوباره اجرا کنید.

3. **چک کردن اتصال اینترنت**:
   - مطمئن بشید اینترنت کار می‌کنه:
     ```bash
     ping 8.8.8.8
     ```
   - اگه جوابی نگرفتید، شبکه‌تون رو چک کنید:
     - روتر یا مودم رو ری‌استارت کنید.
     - اتصال وای‌فای یا اترنت رو بررسی کنید.
     - اگه مشکل ادامه داشت، با ISP تماس بگیرید.

4. **بررسی پورت‌ها**:
   - V2Root از پورت‌های 2300 (HTTP) و 2301 (SOCKS) برای اتصال‌های محلی استفاده می‌کنه. مطمئن بشید بلاک نشدن یا توسط برنامه دیگه‌ای استفاده نشدن.
   - توی لینوکس:
     ```bash
     netstat -tuln | grep 2300
     netstat -tuln | grep 2301
     ```
     اگه پورت‌ها استفاده شدن، برنامه‌ای که ازشون استفاده می‌کنه رو پیدا کنید:
     ```bash
     sudo lsof -i :2300
     ```
     برنامه متداخل رو ببندید یا توی اسکریپتتون پورت‌ها رو عوض کنید (مستندات V2Root رو ببینید).
   - توی ویندوز:
     ```powershell
     netstat -an | findstr 2300
     netstat -an | findstr 2301
     ```
     اگه پورت‌ها مشغولن، برنامه رو توی Task Manager پیدا کنید و ببندید، یا پورت‌ها رو توی اسکریپت عوض کنید.

5. **بررسی تنظیمات فایروال و آنتی‌ویروس**:
   - توی ویندوز، مطمئن بشید Windows Defender یا آنتی‌ویروس دیگه به `test.py` و `libv2root.dll` اجازه کار می‌ده:
     - برید به تنظیمات > آپدیت و امنیت > امنیت ویندوز > محافظت از ویروس و تهدید > مدیریت تنظیمات > استثناها > برای هر دو فایل استثنا اضافه کنید.
   - توی لینوکس، چک کنید `ufw` یا فایروال دیگه پورت‌ها رو بلاک نکرده:
     ```bash
     sudo ufw status
     ```
     پورت‌های V2Root رو باز کنید:
     ```bash
     sudo ufw allow 2300
     sudo ufw allow 2301
     ```
   - تنظیمات فایروال روتر رو چک کنید. اگه پشت NAT هستید، مطمئن بشید پورت‌های 2300 و 2301 فوروارد شدن (راهنمای روترتون رو ببینید).
   - اگه ISP شما پورت‌های VPN رو بلاک می‌کنه، از ارائه‌دهنده VPN یه رشته تنظیمات با پورت دیگه (مثل 443 که کمتر بلاک می‌شه) بخواید.

6. **بررسی فایل لاگ**:
   - فایل `v2root.log` توی پوشه `test.py` رو با ویرایشگر متن باز کنید.
   - دنبال خطاهای مربوط به اتصال یا ساخت `config.json` بگردید، مثل:
     - "Connection refused" (آدرس سرور/پورت اشتباه یا سرور خاموشه).
     - "Network timeout" (مشکل اینترنت یا سرور در دسترس نیست).
     - "Failed to parse config" (یعنی مشکل توی `config.json` ساخته‌شده به خاطر رشته تنظیماته).
   - توی لینوکس لاگ رو ببینید:
     ```bash
     cat v2root.log
     ```
   - اگه لاگ به خطای `config.json` اشاره کرد، یعنی رشته تنظیمات مشکل داره. رشته رو با ارائه‌دهنده VPN دوباره چک کنید.

7. **چک کردن تداخل VPN/پراکسی**:
   - مطمئن بشید VPN یا پراکسی دیگه‌ای (مثل OpenVPN، NordVPN) فعال نیست، چون ممکنه با V2Root تداخل کنه.
   - توی ویندوز، VPN‌های دیگه رو غیرفعال کنید:
     - تنظیمات > شبکه و اینترنت > VPN > هر VPN فعال رو قطع کنید.
   - توی لینوکس، سرویس‌های VPN دیگه رو خاموش کنید:
     ```bash
     sudo systemctl stop openvpn
     ```

8. **به‌روز کردن V2Root و V2Ray**:
   - توی لینوکس، مطمئن بشید V2Ray به‌روزه تا رشته تنظیمات و پروتکل‌های شبکه رو درست مدیریت کنه:
     ```bash
     v2ray --version
     sudo apt update
     sudo apt install v2ray
     ```
   - توی ویندوز، مطمئن بشید آخرین نسخه V2Root رو دارید:
     - پوشه V2Root رو حذف کنید و از https://github.com/V2RayRoot/V2Root/releases دوباره دانلود کنید.
     - چک کنید `libv2root.dll` توی همون پوشه `test.py` یا توی `lib/build_win` باشه.

### هنوز مشکل دارید؟
اگه همه مراحل رو امتحان کردید و هنوز مشکل دارید، ما اینجاییم که کمک کنیم! با این اطلاعات با ما تماس بگیرید:
- فایل اسکریپتی که اجرا می‌کنید (مثل `test.py`).
- رشته تنظیماتی که استفاده می‌کنید (بخش‌های حساس مثل آیدی کاربر رو حذف کنید).
- فایل `v2root.log` توی همون پوشه.
- نوع سیستم عاملتون (ویندوز یا لینوکس).
- مشکل رو گزارش کنید توی:
  - تلگرام: @Sepehr0Day
  - گیت‌هاب: https://github.com/V2RayRoot/V2Root/issues