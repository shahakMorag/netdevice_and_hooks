#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>

#include <stdbool.h>

#include "common.h"
#include "hook_function.h"
#include "network_interface.h"
#include "push_packet_to_interface.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert W. Oliver II");
MODULE_DESCRIPTION("A simple example Linux module.");
MODULE_VERSION("0.01");

struct net_device * g_net_device = NULL;
struct task_struct * g_push_packet_kthread = NULL;

static int __init lkm_example_init(void) {
	if (!add_netdevice(&g_net_device)) {
		return -1;
	}

	if (!push_packet_to_interface_init(&g_push_packet_kthread, g_net_device)) {
		goto cleanup_netdevice;
	}

	if (!hook_function_init()) {
		goto cleanup_kthread;
	}

	return 0;
	
cleanup_kthread:
	push_packet_to_interface_exit(&g_push_packet_kthread);
cleanup_netdevice:
	remove_netdevice(&g_net_device);
	return -1;
}
static void __exit lkm_example_exit(void) {
	push_packet_to_interface_exit(&g_push_packet_kthread);
	remove_netdevice(&g_net_device);
	hook_function_exit();
}

module_init(lkm_example_init);
module_exit(lkm_example_exit);