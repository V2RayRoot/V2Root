# English

## 🛠️ Alternative: Install V2Ray 4.34.0 to Avoid Issues

**Some users have reported encountering errors when running the service or testing latency with V2Ray.**  
It was identified that these issues are caused by a conflict between the library core (version 1.1.1) and the latest V2Ray version (5.31.0). This problem will be fixed in version 1.1.2.  
In the meantime, to avoid these issues, you can follow the steps below to install **V2Ray version 4.34.0**.

### Step 1: Download V2Ray 4.34.0

```bash
cd /tmp
wget https://github.com/v2fly/v2ray-core/releases/download/v4.34.0/v2ray-linux-64.zip
```

### Step 2: Install `unzip` and extract files

For **Ubuntu/Debian**:
```bash
sudo apt install unzip -y
```

For **Arch Linux**:
```bash
sudo pacman -S unzip
```

```bash
unzip v2ray-linux-64.zip
```

### Step 3: Install binaries and config files

```bash
sudo mkdir -p /usr/local/bin /usr/local/share/v2ray /etc/v2ray
sudo install -m 755 v2ray v2ctl /usr/local/bin/
sudo install -m 644 geo* /usr/local/share/v2ray/
sudo install -m 644 config.json /etc/v2ray/config.json
```

### Step 4: Create systemd service

```bash
sudo tee /etc/systemd/system/v2ray.service > /dev/null <<EOF
[Unit]
Description=V2Ray Service
After=network.target

[Service]
ExecStart=/usr/local/bin/v2ray -config /etc/v2ray/config.json
Restart=on-failure
User=nobody
CapabilityBoundingSet=CAP_NET_ADMIN CAP_NET_BIND_SERVICE
AmbientCapabilities=CAP_NET_ADMIN CAP_NET_BIND_SERVICE
NoNewPrivileges=true

[Install]
WantedBy=multi-user.target
EOF
```

### Step 5: Enable and start the service

```bash
sudo systemctl daemon-reexec
sudo systemctl enable v2ray
sudo systemctl start v2ray
```

---

# فارسی

## 🛠️ نصب V2Ray نسخه 4.34.0 برای رفع مشکلات

**برخی از کاربران هنگام اجرای سرویس یا تست تاخیر با ارورهایی از سمت V2Ray مواجه شده‌اند.**  
مشخص شد که این مشکلات به دلیل تداخل هسته کتابخانه (ورژن ۱.۱.۱) با آخرین نسخه فعلی V2Ray (ورژن ۵.۳۱.۰) رخ می‌دهد. این مشکل در نسخه ۱.۱.۲ رفع خواهد شد.  
اما در حال حاضر برای رفع این مشکلات، می‌توانید از راهنمای زیر برای نصب **V2Ray نسخه ۴.۳۴.۰** استفاده کنید.

### گام ۱: دانلود V2Ray نسخه 4.34.0

```bash
cd /tmp
wget https://github.com/v2fly/v2ray-core/releases/download/v4.34.0/v2ray-linux-64.zip
```

### گام ۲: نصب `unzip` و استخراج فایل‌ها

برای **اوبونتو/دبیان**:
```bash
sudo apt install unzip -y
```

برای **آرچ لینوکس**:
```bash
sudo pacman -S unzip
```

```bash
unzip v2ray-linux-64.zip
```

### گام ۳: نصب باینری‌ها و فایل‌های تنظیمات

```bash
sudo mkdir -p /usr/local/bin /usr/local/share/v2ray /etc/v2ray
sudo install -m 755 v2ray v2ctl /usr/local/bin/
sudo install -m 644 geo* /usr/local/share/v2ray/
sudo install -m 644 config.json /etc/v2ray/config.json
```

### گام ۴: ایجاد سرویس systemd

```bash
sudo tee /etc/systemd/system/v2ray.service > /dev/null <<EOF
[Unit]
Description=V2Ray Service
After=network.target

[Service]
ExecStart=/usr/local/bin/v2ray -config /etc/v2ray/config.json
Restart=on-failure
User=nobody
CapabilityBoundingSet=CAP_NET_ADMIN CAP_NET_BIND_SERVICE
AmbientCapabilities=CAP_NET_ADMIN CAP_NET_BIND_SERVICE
NoNewPrivileges=true

[Install]
WantedBy=multi-user.target
EOF
```

### گام ۵: فعال‌سازی و شروع سرویس

```bash
sudo systemctl daemon-reexec
sudo systemctl enable v2ray
sudo systemctl start v2ray
```

---
