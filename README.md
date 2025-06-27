Dưới đây là phiên bản đã chỉnh sửa của file README để phù hợp với **thuật toán mã hóa/giải mã Caesar** thay vì AES-256-CBC:

---

# USB Crypto Driver - Hệ thống mã hóa/giải mã Caesar

## 🛡️ Tổng quan

**USB Crypto Driver** là hệ thống bảo mật đơn giản sử dụng thuật toán **Caesar Cipher** để mã hóa và giải mã tệp tin, chỉ hoạt động khi thiết bị USB hợp lệ được kết nối.

Hệ thống bao gồm:

* **USB Kernel Driver**: Tích hợp thuật toán Caesar Cipher
* **Ứng dụng người dùng**: Dễ sử dụng qua dòng lệnh
* **Giao diện `/dev`**: Kết nối giữa ứng dụng và kernel module

## 🏗️ Kiến trúc hệ thống

```
[Ứng dụng người dùng] 
        ↓
[/dev/usb_crypto] 
        ↓
[USB Crypto Driver] ← [USB Device]
        ↓
[Caesar Encryption/Decryption]
```

## ⚙️ Tính năng chính

* ✅ Mã hóa/giải mã bằng **Caesar Cipher** với độ lệch có thể tùy chỉnh
* ✅ Chỉ hoạt động khi thiết bị USB được nhận diện
* ✅ Giao diện dòng lệnh trực quan
* ✅ Hỗ trợ tệp văn bản kích thước vừa và nhỏ
* ✅ Tự động phát hiện thiết bị USB
* ✅ Dễ tích hợp và thử nghiệm cho mục đích giáo dục/bảo mật đơn giản

## 🧰 Yêu cầu hệ thống

### Phần cứng

* Linux OS (kernel 4.0+)
* Cổng và thiết bị USB

### Phần mềm

* GCC, Make, Linux kernel headers
* Quyền root để nạp driver

### Cài đặt dependencies

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

**CentOS/RHEL:**

```bash
sudo yum groupinstall "Development Tools"
sudo yum install kernel-devel
```

## 🔧 Cài đặt và sử dụng

### 1. Chuẩn bị mã nguồn

```bash
mkdir usb-caesar-driver
cd usb-caesar-driver

# Copy các file: usb_crypto_driver.c, crypto_app.c, Makefile, build.sh
```

### 2. Build hệ thống

```bash
chmod +x build.sh
sudo ./build.sh build
```

### 3. Load driver

```bash
sudo ./build.sh load
```

### 4. Kiểm tra trạng thái

```bash
sudo ./build.sh status
./crypto_app -s
```

### 5. Kết nối USB

Cắm thiết bị USB. Driver sẽ kiểm tra và kích hoạt tính năng mã hóa nếu đúng VID/PID.

#### 5.1 Khắc phục lỗi không nhận thiết bị

Tạo file udev rule như sau:

```bash
sudo gedit /etc/udev/rules.d/99-usb-crypto.rules
```

Thêm:

```bash
ACTION=="add", SUBSYSTEM=="usb", ATTRS{idVendor}=="xxxx", ATTRS{idProduct}=="yyyy", RUN+="/bin/sh -c 'echo 0 > /sys/bus/usb/devices/%k/driver/unbind; modprobe -r uas usb_storage'"
```

*(Thay `xxxx` và `yyyy` bằng VID/PID tương ứng)*

### 6. Sử dụng ứng dụng mã hóa/giải mã

```bash
# Mã hóa với Caesar (mặc định shift = 3)
./crypto_app -e input.txt encrypted.txt

# Giải mã
./crypto_app -d encrypted.txt decrypted.txt

# Mã hóa với shift tùy chọn
./crypto_app -e -k 5 input.txt encrypted.txt

# Trợ giúp
./crypto_app -h
```

## 🧪 Các lệnh quản lý

### build.sh

```bash
sudo ./build.sh build     # Biên dịch
sudo ./build.sh load      # Load driver
sudo ./build.sh unload    # Gỡ driver
sudo ./build.sh reload    # Tải lại
./build.sh status         # Trạng thái
sudo ./build.sh install   # Cài vĩnh viễn
sudo ./build.sh uninstall # Gỡ cài đặt
./build.sh clean          # Dọn file build
```

### Thủ công

```bash
sudo insmod usb_crypto_driver.ko
sudo rmmod usb_crypto_driver
modinfo usb_crypto_driver.ko
lsmod | grep usb_crypto
```

## 🔧 Tùy chỉnh thiết bị USB

```bash
lsusb  # Lấy VID và PID
```

Sửa file `usb_crypto_driver.c`:

```c
{ USB_DEVICE(0xABCD, 0x1234) },
```

Sau đó:

```bash
sudo ./build.sh reload
```

## 🔍 Debug & Logging

```bash
dmesg | tail -20
sudo tail -f /var/log/kern.log | grep usb_crypto
cat /proc/usb_crypto
```

## 🔐 Bảo mật và giới hạn

* **Caesar Cipher** chỉ phù hợp cho mục đích giáo dục, kiểm thử hoặc hệ thống đơn giản.
* **Không nên sử dụng trong môi trường yêu cầu bảo mật nghiêm ngặt.**

---

✅ **Gợi ý mở rộng**:

* Nâng cấp thuật toán (Vigenère, XOR, AES...)
* Tăng tùy chọn cấu hình (key từ USB, IV tự sinh, kiểm tra checksum)

---

Nếu bạn muốn mình tạo thêm hướng dẫn tiếng Anh hoặc tài liệu chi tiết cho sinh viên, chỉ cần yêu cầu!
