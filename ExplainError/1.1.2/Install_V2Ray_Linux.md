# Installing the Latest V2Ray on Linux

## English

This guide explains how to manually install the latest V2Ray core from https://github.com/v2fly/v2ray-core/releases on various Linux distributions. Follow the steps for your system to ensure V2Root works correctly. These instructions are for **Debian/Ubuntu-based**, **Arch/Arch-based**, **Fedora**, **CentOS/RHEL**, and **Ubuntu WSL** environments.

### Prerequisites
- A terminal with internet access.
- `sudo` or root access.
- Basic tools: `wget`, `unzip`, `curl` (install if missing, see below).
- At least 500MB free disk space.

### General Steps for All Distributions
1. **Check for Existing V2Ray**:
   ```bash
   v2ray version
   ```
   If installed, note the version. If outdated or missing, proceed.

2. **Download the Latest V2Ray**:
   - Visit https://github.com/v2fly/v2ray-core/releases.
   - Find the latest release (e.g., `v5.12.1`) and copy the link for the Linux 64-bit binary (e.g., `v2ray-linux-64.zip`).
   - Download it:
     ```bash
     wget https://github.com/v2fly/v2ray-core/releases/download/v5.31.0/v2ray-linux-64.zip
     ```
     Replace `v5.12.1` and `v2ray-linux-64.zip` with the latest version and file name.

3. **Extract the Archive**:
   ```bash
   unzip v2ray-linux-64.zip -d v2ray
   ```
   This creates a `v2ray` folder with files like `v2ray` and `v2ctl`.

4. **Move to System Path**:
   ```bash
   sudo mv v2ray/v2ray /usr/local/bin/
   sudo mv v2ray/v2ctl /usr/local/bin/
   ```
   Ensure executable:
   ```bash
   sudo chmod +x /usr/local/bin/v2ray
   sudo chmod +x /usr/local/bin/v2ctl
   ```

5. **Verify Installation**:
   ```bash
   v2ray version
   ```
   You should see the latest version (e.g., `V2Ray 5.12.1`).

### Specific Instructions by Distribution

#### Debian/Ubuntu-based (e.g., Ubuntu, Debian, Linux Mint)
1. **Install Tools**:
   ```bash
   sudo apt update
   sudo apt install wget unzip curl
   ```

2. **Follow General Steps** (above).
   - If `unzip` fails, ensure it’s installed:
     ```bash
     sudo apt install unzip
     ```

3. **Optional: Set Up Service**:
   - Copy the systemd service file:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - Enable and start:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - Check status:
     ```bash
     sudo systemctl status v2ray
     ```

#### Arch/Arch-based (e.g., Arch Linux, Manjaro)
1. **Install Tools**:
   ```bash
   sudo pacman -Syu
   sudo pacman -S wget unzip curl
   ```

2. **Follow General Steps**.
   - If `wget` or `unzip` is missing:
     ```bash
     sudo pacman -S wget unzip
     ```

3. **Optional: Set Up Service**:
   - Copy service file:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - Enable/start:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - Check:
     ```bash
     sudo systemctl status v2ray
     ```

#### Fedora
1. **Install Tools**:
   ```bash
   sudo dnf update
   sudo dnf install wget unzip curl
   ```

2. **Follow General Steps**.
   - If tools are missing:
     ```bash
     sudo dnf install wget unzip
     ```

3. **Optional: Set Up Service**:
   - Copy service file:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - Enable/start:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - Check:
     ```bash
     sudo systemctl status v2ray
     ```

#### CentOS/RHEL
1. **Install Tools**:
   ```bash
   sudo yum update
   sudo yum install wget unzip curl
   ```

2. **Follow General Steps**.
   - If `unzip` is missing:
     ```bash
     sudo yum install unzip
     ```

3. **Optional: Set Up Service**:
   - Copy service file:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - Enable/start:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - Check:
     ```bash
     sudo systemctl status v2ray
     ```

#### Ubuntu WSL (Windows Subsystem for Linux)
1. **Install Tools**:
   ```bash
   sudo apt update
   sudo apt install wget unzip curl
   ```

2. **Follow General Steps**.
   - Note: WSL doesn’t support systemd, so skip service setup.
   - Run V2Ray manually if needed:
     ```bash
     /usr/local/bin/v2ray run
     ```

3. **Check Installation**:
   ```bash
   v2ray version
   ```

### Troubleshooting
- **Download Fails**: Ensure internet works (`ping 8.8.8.8`). Try a different release link.
- **Permission Denied**: Use `sudo` for commands or check file permissions:
  ```bash
  sudo chmod +x /usr/local/bin/v2ray
  ```
- **V2Ray Not Found**: Verify `/usr/local/bin/v2ray` exists:
  ```bash
  ls /usr/local/bin/v2ray
  ```
- **Still Stuck?** Contact support with:
  - Your Linux distribution.
  - Output of `v2ray version`.
  - Any error messages.
  - Telegram: @Sepehr0Day
  - GitHub: https://github.com/V2RayRoot/V2Root/issues

---

## Persian (فارسی)

# نصب آخرین نسخه V2Ray روی لینوکس

این راهنما توضیح می‌ده چطور آخرین نسخه هسته V2Ray رو از https://github.com/v2fly/v2ray-core/releases به‌صورت دستی روی توزیع‌های مختلف لینوکس نصب کنید. مراحل رو برای سیستم خودتون دنبال کنید تا V2Root درست کار کنه. این دستورات برای **دبیان/اوبونتو-بیس**، **آرچ/آرچ-بیس**، **فدورا**، **سنت‌اواس/رد هت** و **اوبونتو WSL** هستن.

### پیش‌نیازها
- ترمینال با دسترسی اینترنت.
- دسترسی `sudo` یا روت.
- ابزارهای پایه: `wget`، `unzip`، `curl` (اگه نیستن، پایین نصب کنید).
- حداقل ۵۰۰ مگ فضای خالی.

### مراحل کلی برای همه توزیع‌ها
1. **چک کردن V2Ray موجود**:
   ```bash
   v2ray version
   ```
   اگه نصبه، نسخه رو ببینید. اگه قدیمی یا غایبه، ادامه بدید.

2. **دانلود آخرین V2Ray**:
   - برید به https://github.com/v2fly/v2ray-core/releases.
   - آخرین نسخه (مثل `v5.12.1`) رو پیدا کنید و لینک باینری ۶۴ بیتی لینوکس (مثل `v2ray-linux-64.zip`) رو کپی کنید.
   - دانلود:
     ```bash
     wget https://github.com/v2fly/v2ray-core/releases/download/v5.31.0/v2ray-linux-64.zip
     ```
     `v5.12.1` و `v2ray-linux-64.zip` رو با نسخه و اسم فایل جدید عوض کنید.

3. **استخراج فایل**:
   ```bash
   unzip v2ray-linux-64.zip -d v2ray
   ```
   این یه پوشه `v2ray` با فایل‌هایی مثل `v2ray` و `v2ctl` می‌سازه.

4. **انتقال به مسیر سیستمی**:
   ```bash
   sudo mv v2ray/v2ray /usr/local/bin/
   sudo mv v2ray/v2ctl /usr/local/bin/
   ```
   مطمئن بشید قابل اجرا هستن:
   ```bash
   sudo chmod +x /usr/local/bin/v2ray
   sudo chmod +x /usr/local/bin/v2ctl
   ```

5. **بررسی نصب**:
   ```bash
   v2ray version
   ```
   باید نسخه جدید رو ببینید (مثل `V2Ray 5.12.1`).

### دستورات خاص برای هر توزیع

#### دبیان/اوبونتو-بیس (مثل اوبونتو، دبیان، لینوکس مینت)
1. **نصب ابزارها**:
   ```bash
   sudo apt update
   sudo apt install wget unzip curl
   ```

2. **مراحل کلی** (بالا) رو دنبال کنید.
   - اگه `unzip` کار نکرد، نصب کنید:
     ```bash
     sudo apt install unzip
     ```

3. **اختیاری: راه‌اندازی سرویس**:
   - فایل سرویس رو کپی کنید:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - فعال و اجرا کنید:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - وضعیت:
     ```bash
     sudo systemctl status v2ray
     ```

#### آرچ/آرچ-بیس (مثل آرچ لینوکس، مانجارو)
1. **نصب ابزارها**:
   ```bash
   sudo pacman -Syu
   sudo pacman -S wget unzip curl
   ```

2. **مراحل کلی**.
   - اگه `wget` یا `unzip` نیست:
     ```bash
     sudo pacman -S wget unzip
     ```

3. **اختیاری: راه‌اندازی سرویس**:
   - کپی فایل سرویس:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - فعال/اجرا:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - چک:
     ```bash
     sudo systemctl status v2ray
     ```

#### فدورا
1. **نصب ابزارها**:
   ```bash
   sudo dnf update
   sudo dnf install wget unzip curl
   ```

2. **مراحل کلی**.
   - اگه ابزارها نیستن:
     ```bash
     sudo dnf install wget unzip
     ```

3. **اختیاری: راه‌اندازی سرویس**:
   - کپی فایل سرویس:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - فعال/اجرا:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - چک:
     ```bash
     sudo systemctl status v2ray
     ```

#### سنت‌اواس/رد هت
1. **نصب ابزارها**:
   ```bash
   sudo yum update
   sudo yum install wget unzip curl
   ```

2. **مراحل کلی**.
   - اگه `unzip` نیست:
     ```bash
     sudo yum install unzip
     ```

3. **اختیاری: راه‌اندازی سرویس**:
   - کپی فایل سرویس:
     ```bash
     sudo cp v2ray/systemd/system/v2ray.service /etc/systemd/system/
     ```
   - فعال/اجرا:
     ```bash
     sudo systemctl enable v2ray
     sudo systemctl start v2ray
     ```
   - چک:
     ```bash
     sudo systemctl status v2ray
     ```

#### اوبونتو WSL (ویندوز ساب‌سیستم لینوکس)
1. **نصب ابزارها**:
   ```bash
   sudo apt update
   sudo apt install wget unzip curl
   ```

2. **مراحل کلی**.
   - نکته: WSL از systemd پشتیبانی نمی‌کنه، پس سرویس راه‌اندازی نکنید.
   - V2Ray رو دستی اجرا کنید اگه لازم بود:
     ```bash
     /usr/local/bin/v2ray run
     ```

3. **بررسی نصب**:
   ```bash
   v2ray version
   ```

### عیب‌یابی
- **دانلود نشد**: اینترنت رو چک کنید (`ping 8.8.8.8`). لینک نسخه دیگه رو امتحان کنید.
- **خطای دسترسی**: از `sudo` استفاده کنید یا دسترسی رو درست کنید:
  ```bash
  sudo chmod +x /usr/local/bin/v2ray
  ```
- **V2Ray پیدا نشد**: چک کنید `/usr/local/bin/v2ray` هست:
  ```bash
  ls /usr/local/bin/v2ray
  ```
- **هنوز مشکل دارید؟** با اینا با پشتیبانی تماس بگیرید:
  - توزیع لینوکس شما.
  - خروجی `v2ray version`.
  - پیام‌های خطا.
  - تلگرام: @Sepehr0Day
  - گیت‌هاب: https://github.com/V2RayRoot/V2Root/issues