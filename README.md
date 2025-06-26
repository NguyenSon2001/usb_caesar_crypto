# USB Crypto Driver - Hệ thống mã hóa/giải mã tệp tin

## Tổng quan

Hệ thống USB Crypto Driver là một giải pháp bảo mật cho phép mã hóa và giải mã tệp tin chỉ khi có USB device được kết nối. Hệ thống bao gồm:

- **USB Kernel Driver**: Chứa thuật toán mã hóa AES-256-CBC
- **Ứng dụng người dùng**: Giao diện để thực hiện mã hóa/giải mã
- **Proc interface**: Giao tiếp giữa ứng dụng và driver

## Kiến trúc hệ thống

```
[Ứng dụng người dùng] 
        ↓
[/proc/usb_crypto] 
        ↓
[USB Crypto Driver] ← [USB Device]
        ↓
[AES Encryption/Decryption]
```

## Tính năng chính

- ✅ Mã hóa AES-256-CBC với IV ngẫu nhiên
- ✅ Chỉ hoạt động khi USB device được kết nối
- ✅ Giao diện dòng lệnh đơn giản
- ✅ Hỗ trợ tệp tin có kích thước lớn
- ✅ Tự động phát hiện kết nối/ngắt kết nối USB
- ✅ Logging và giám sát trạng thái

## Yêu cầu hệ thống

### Phần cứng
- Máy tính chạy Linux (kernel 4.0+)
- USB port
- USB device (flash drive, external drive, etc.)

### Phần mềm
- Linux kernel headers
- GCC compiler
- Make utility
- Root privileges để load driver

### Cài đặt dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential linux-headers-$(uname -r)
```

**CentOS/RHEL:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install kernel-devel
```

## Cài đặt và sử dụng

### 1. Tải và chuẩn bị mã nguồn

```bash
# Tạo thư mục dự án
mkdir usb-crypto-driver
cd usb-crypto-driver

# Copy các file mã nguồn vào thư mục này:
# - usb_crypto_driver.c
# - crypto_app.c  
# - Makefile
# - build.sh
```

### 2. Build hệ thống

```bash
# Cấp quyền thực thi cho script build
chmod +x build.sh

# Build toàn bộ hệ thống
sudo ./build.sh build
```

### 3. Load driver

```bash
# Load driver vào kernel
sudo ./build.sh load
```

### 4. Kiểm tra trạng thái

```bash
# Kiểm tra trạng thái hệ thống
sudo ./build.sh status

# Hoặc sử dụng ứng dụng
./crypto_app -s
```

### 5. Kết nối USB device

Cắm USB device vào máy tính. Driver sẽ tự động phát hiện và cho phép thực hiện mã hóa/giải mã.


### 5.1 Fix lỗi không nhận thiết bị
- Nguyên nhân : Do một driver khác đã chiếm quyền với thiết bị USB ==> Cần hủy liên kết giữa USB với driver đó trước
- Nếu thiết bị USB là thẻ nhớ thì :

```bash
# Mở file config
sudo gedit /etc/udev/rules.d/99-usb-crypto.rules
```
```bash
# Thêm nội dung
ACTION=="add", SUBSYSTEM=="usb", ATTRS{idVendor}=="05e3", ATTRS{idProduct}=="0747", RUN+="/bin/sh -c 'echo 0 > /sys/bus/usb/devices/%k/driver/unbind; modprobe -r uas usb_storage'"
```
- Nhớ sửa lại idVendor và idProduct
- Mục đích của nội dung được thêm vào file: Ngăn không cho kernel tự gán driver USB mặc định (như usb_storage hoặc uas) cho thiết bị USB có VID:PID là 05e3:0747.
###
### 6. Sử dụng ứng dụng mã hóa

```bash
# Mã hóa tệp tin
./crypto_app -e input.txt encrypted.dat

# Giải mã tệp tin  
./crypto_app -d encrypted.dat decrypted.txt

# Xem trạng thái
./crypto_app -s

# Xem help
./crypto_app -h
```

## Các lệnh quản lý

### Script build.sh

```bash
# Build mã nguồn
sudo ./build.sh build

# Load driver
sudo ./build.sh load

# Unload driver
sudo ./build.sh unload

# Reload driver
sudo ./build.sh reload

# Test hệ thống
sudo ./build.sh test

# Xem trạng thái
./build.sh status

# Cài đặt vĩnh viễn
sudo ./build.sh install

# Gỡ cài đặt
sudo ./build.sh uninstall

# Clean build files
./build.sh clean
```

### Quản lý driver thủ công

```bash
# Load driver
sudo insmod usb_crypto_driver.ko

# Unload driver
sudo rmmod usb_crypto_driver

# Xem thông tin driver
modinfo usb_crypto_driver.ko

# Kiểm tra driver đã load
lsmod | grep usb_crypto
```

## Tùy chỉnh USB Device ID

Mặc định, driver hỗ trợ generic USB storage devices. Để chỉ định USB device cụ thể:

1. Xem thông tin USB device:
```bash
lsusb
# Tìm Vendor ID và Product ID
```

2. Sửa file `usb_crypto_driver.c`:
```c
static struct usb_device_id usb_crypto_table[] = {
    { USB_DEVICE(0x1234, 0x5678) }, // Thay bằng VID/PID của bạn
    {}
};
```

3. Rebuild và reload driver:
```bash
sudo ./build.sh reload
```

## Kiểm tra và Debug

### Xem log của driver

```bash
# Xem kernel messages
dmesg | tail -20

# Theo dõi real-time
sudo tail -f /var/log/kern.log | grep usb_crypto
```

### Kiểm tra proc interface

```bash
# Xem trạng thái driver
cat /proc/usb_crypto

# Test gửi lệnh (chỉ dành cho debug)
echo "E:test data" | sudo tee /proc/usb_crypto
```

### Kiểm tra USB devices

```bash
# Liệt kê USB devices
lsusb

# Xem chi tiết USB device
lsusb -v -d [vendor_id]:[product_id]

# Giám sát USB events
sudo udevadm monitor --udev
```

## Bảo mật

### Key Management
- Hiện tại key