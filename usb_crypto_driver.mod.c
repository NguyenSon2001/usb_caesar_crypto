#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x8f30bf67, "module_layout" },
	{ 0xf172ae21, "param_ops_int" },
	{ 0xdbf61b3b, "no_llseek" },
	{ 0x71a26cc2, "usb_deregister" },
	{ 0xc5850110, "printk" },
	{ 0x3ec8d84f, "usb_register_driver" },
	{ 0xac586286, "_dev_err" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0xafc2b198, "usb_register_dev" },
	{ 0x60c9d98f, "__stack_chk_fail" },
	{ 0x2276db98, "kstrtoint" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x1d07e365, "memdup_user_nul" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x7e2b798f, "_dev_info" },
	{ 0x37a0cba, "kfree" },
	{ 0x91a3cfc0, "usb_deregister_dev" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v05E3p0747d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "51D8A47111118E930426B19");
