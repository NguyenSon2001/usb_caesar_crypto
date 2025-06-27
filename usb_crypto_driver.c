/*======================================================================
 * Driver USB Caesar Cipher (Mã hoá/giải mã)
 *---------------------------------------------------------------------
 *  Hướng dẫn nhanh (ghi chú):
 *  - Thiết bị ký tự xuất hiện dưới /dev/usb/cryptoX (X = minor).
 *  - Ghi (write) vào thiết bị theo cú pháp:
 *       "E:<dữ_liệu>"      → Mã hoá (E = "Mã hoá")
 *       "D:<dữ_liệu>"      → Giải mã (D = "Giải")
 *       "S:<số_0‑25>"      → Thay đổi (S = "Thay đổi") độ dịch Caesar
 *  - Đọc (read) sẽ trả lại kết quả mã hoá / giải mã gần nhất.
 *---------------------------------------------------------------------
 *  Biên dịch & nạp module:
 *      $ make -C /lib/modules/$(uname -r)/build M=$PWD modules
 *      # insmod usb_crypto_vi.ko dich=5   # đặt độ dịch mặc định = 5
 *----------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME            "usb_crypto"
#define DRIVER_VERSION         "0.1"
#define BUFFER_SIZE            (64 * 1024)   /* bộ đệm tối đa */
#define CAESAR_DEFAULT_SHIFT   13            /* dịch mặc định */
#define USB_CRYPTO_MINOR_BASE  192           /* minor động đầu tiên *//*Dựa theo quy ước USB*/

/*────────────────── Biến module (chỉ 1 thiết bị) ──────────────────*/
static struct usb_device *g_usb_dev;     /* thiết bị USB đang gắn */
static u8   *g_buf_kq;                   /* kết quả mã/giải gần nhất */
static size_t g_buf_len;                 /* số byte hợp lệ trong g_buf_kq */
static int  caesar_shift = CAESAR_DEFAULT_SHIFT; /* độ dịch hiện tại */

/* Cho phép thay đổi giá trị dịch khi insmod: insmod ... dich=<n> */
module_param_named(dich, caesar_shift, int, 0444);
MODULE_PARM_DESC(dich, "Caesar shift: (0‑25)");

/*────────────────── Các hàm Caesar helper ─────────────────────────*/
static inline char caesar_apply(char ch, int shift)
{
    if (ch >= 'A' && ch <= 'Z')// nếu ký tự là chữ hoa
        return ((ch - 'A' + shift) % 26) + 'A';// áp dụng dịch theo shift
    if (ch >= 'a' && ch <= 'z')
        return ((ch - 'a' + shift) % 26) + 'a';
    return ch;
}

static void caesar_transform(const u8 *in, u8 *out, size_t len, int shift)
{
    size_t i;
    for (i = 0; i < len; ++i)// lặp qua từng ký tự
        out[i] = caesar_apply(in[i], shift);// áp dụng hàm Caesar
}

/*────────────────── File‑ops cho thiết bị ký tự ───────────────────*/
static int usb_crypto_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t usb_crypto_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    size_t todo;

    if (!g_buf_kq || *ppos >= g_buf_len)// nếu không có dữ liệu hoặc đã đọc hết
        return 0; /* EOF */

    todo = min(count, g_buf_len - (size_t)*ppos);// tính số byte cần đọc = min(count, số byte còn lại)
    if (copy_to_user(buf, g_buf_kq + *ppos, todo)) // sao chép dữ liệu từ kernel space sang user space
        return -EFAULT;

    *ppos += todo;// cập nhật vị trí đọc
    return todo;
}

static ssize_t usb_crypto_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char *kbuf = NULL;   /* bản sao tạm, thêm NUL cuối chuỗi */
    char op;             /* ký tự lệnh */
    size_t data_len;
    int new_shift, err = 0;
    u8 *tmp;

    if (count < 3 || count > BUFFER_SIZE)
        return -EINVAL;

    kbuf = memdup_user_nul(buf, count); // sao chép dữ liệu từ user space
    if (IS_ERR(kbuf))
        return PTR_ERR(kbuf);

    op = kbuf[0];// ký tự lệnh đầu tiên
    if (kbuf[1] != ':') {// kiểm tra định dạng lệnh
        err = -EINVAL;
        goto out_free;
    }

    /* Lệnh "s:<num>" — thay đổi độ dịch */
    if (op == 's' || op == 'S') {
        err = kstrtoint(kbuf + 2, 10, &new_shift);// chuyển đổi chuỗi sang số nguyên
        if (err || new_shift < 0 || new_shift > 25) {// kiểm tra giá trị hợp lệ
            err = -EINVAL;
            goto out_free;
        }
        caesar_shift = new_shift;// cập nhật độ dịch Caesar
        err = count; /* coi như đã ghi hết */
        goto out_free;
    }

    /* Lệnh "e:" (Mã hoá) hoặc "d:" (Giải mã) theo sau là dữ liệu */
    data_len = count - 2;// độ dài dữ liệu sau ký tự lệnh và dấu hai chấm
    if (!data_len) {// nếu không có dữ liệu
        err = -EINVAL;
        goto out_free;
    }

    tmp = kmalloc(data_len, GFP_KERNEL);// cấp phát bộ nhớ tạm
    if (!tmp) {
        err = -ENOMEM;
        goto out_free;
    }

    switch (op) {// Mã hoá hoặc Giải mã
    case 'E':// Mã hoá
    case 'e':
        caesar_transform((u8 *)kbuf + 2, tmp, data_len, caesar_shift);// thực hiện mã hoá theo công thức Caesar: (x + shift) mod 26, mod là phép chia lấy dư ví dụ với ký tự h là 7 thì với shift = 5 sẽ là (7 + 5) mod 26 = 12, ký tự thứ 12 là l = 'l'
        break;
    case 'D':// Giải mã
    case 'd':
        caesar_transform((u8 *)kbuf + 2, tmp, data_len, (26 - caesar_shift) % 26);// thực hiện giải mã theo công thức Caesar: (x - shift + 26) mod 26 tương tự với ký tự l là 11 thì với shift = 5 sẽ là (11 - 5 + 26) mod 26 = 12, ký tự thứ 12 là h = 'h'
        break;
    default:// nếu lệnh không hợp lệ
        kfree(tmp);
        err = -EINVAL;
        goto out_free;
    }

    kfree(g_buf_kq);// giải phóng bộ đệm cũ
    g_buf_kq = tmp;// gán bộ đệm mới
    g_buf_len = data_len;// cập nhật độ dài dữ liệu
    err = count; // coi như đã ghi hết

out_free:
    kfree(kbuf);
    return err;
}

static const struct file_operations usb_crypto_fops = {
    .owner   = THIS_MODULE,
    .open    = usb_crypto_open,
    .read    = usb_crypto_read,
    .write   = usb_crypto_write,
    .llseek  = no_llseek,
};

/*────────────────── USB glue ───────────────────────────────────────*/
static struct usb_class_driver crypto_class; /* điền trong usb_crypto_probe() */

static int usb_crypto_probe(struct usb_interface *iface,
                            const struct usb_device_id *id)// Khi thiết bị USB được kết nối
{
    int retval;

    g_usb_dev = interface_to_usbdev(iface);

    crypto_class.name       = "usb/crypto%d";  /* /dev/usb/cryptoX */
    crypto_class.fops       = &usb_crypto_fops;
    crypto_class.minor_base = USB_CRYPTO_MINOR_BASE;

    retval = usb_register_dev(iface, &crypto_class);
    if (retval) {
        dev_err(&iface->dev, "Could not get a minor number: %d\n", retval);
        return retval;
    }

    dev_info(&iface->dev, "USB-Crypto device attached with minor %d\n",
             iface->minor);

    g_buf_kq = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!g_buf_kq) {
        usb_deregister_dev(iface, &crypto_class);
        return -ENOMEM;
    }
    g_buf_len = 0;

    return 0;
}

static void usb_crypto_disconnect(struct usb_interface *iface)// Khi thiết bị USB bị ngắt kết nối
{
    usb_deregister_dev(iface, &crypto_class);

    kfree(g_buf_kq);
    g_buf_kq = NULL;
    g_buf_len = 0;
    g_usb_dev = NULL;

    dev_info(&iface->dev, "USB-Crypto device disconnected\n");
}

/*────────────────── Bảng VID:PID thiết bị hỗ trợ ──────────────────*/
static const struct usb_device_id crypto_id_table[] = {
    { USB_DEVICE(0x05e3, 0x0747) }, /* Thay bằng VID:PID thực tế */
    { } /* kết thúc bảng */
};
MODULE_DEVICE_TABLE(usb, crypto_id_table);

/*────────────────── Cấu trúc usb_driver ───────────────────────────*/
static struct usb_driver usb_crypto_driver = {
    .name       = DRIVER_NAME,
    .probe      = usb_crypto_probe,
    .disconnect = usb_crypto_disconnect,
    .id_table   = crypto_id_table,
};

/*────────────────── Hàm init/exit module ──────────────────────────*/
static int __init usb_crypto_init(void)// Hàm khởi tạo module, khởi chạy khi nạp module bằng insmod
{
    int ret = usb_register(&usb_crypto_driver);// đăng ký driver USB, thông số usb_crypto_driver đã định nghĩa ở trên gồm các hàm probe, disconnect, id_table, name...
    if (ret)
        pr_err(DRIVER_NAME ": usb_register failed: %d\n", ret);
    else
        pr_info(DRIVER_NAME ": driver loaded (v" DRIVER_VERSION ")\n");
    return ret;
}

static void __exit usb_crypto_exit(void)// Hàm giải phóng module, chạy khi rút module bằng rmmod
{
    usb_deregister(&usb_crypto_driver);
    pr_info(DRIVER_NAME ": driver unloaded\n");
}
module_init(usb_crypto_init);
module_exit(usb_crypto_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nhóm");
MODULE_DESCRIPTION("USB Caesar Cipher char driver");
MODULE_VERSION(DRIVER_VERSION);
