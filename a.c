#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/if_arp.h>
#include <linux/kthread.h>
#include <linux/rtnetlink.h>
#include <linux/netdevice.h>

#include <stdbool.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert W. Oliver II");
MODULE_DESCRIPTION("A simple example Linux module.");
MODULE_VERSION("0.01");

#define UNUSED(a) do { (void)a } while (0)

struct net_device * g_net_device = NULL;

struct net_device_ops ndo = {
	0
};

void netdevice_setup(struct net_device * dev) {
	int j = 0;
	dev->type = ARPHRD_IEEE80211;

	for (; j < ETH_ALEN; ++j) {
		dev->dev_addr[j] = (char)j;
	}

	ether_setup(dev);
	dev->netdev_ops = &ndo;
}

static bool add_netdevice(void) {
	g_net_device = alloc_netdev(0, "bb%d", NET_NAME_UNKNOWN, netdevice_setup);
	if (NULL == g_net_device) {
		printk("g_net_device alloc failed!\n");
		return false;
	}

	if (0 > dev_alloc_name(g_net_device, g_net_device->name)) {
		printk("dev_alloc_name failed!\n");
		goto cleaup;
	}

	if (0 > register_netdev(g_net_device)) {
		printk("failed to register net device\n");
		goto cleaup;
	}

	return true;

cleaup:
	free_netdev(g_net_device);
	g_net_device = NULL;
	return false;
}

void remove_netdevice(void) {
	if (NULL != g_net_device) {
		unregister_netdev(g_net_device);
		free_netdev(g_net_device);
	}
}

static int __init lkm_example_init(void) {
	if (!add_netdevice()) {
		return -1;
	}

	return 0;
}
static void __exit lkm_example_exit(void) {
	remove_netdevice();
}

module_init(lkm_example_init);
module_exit(lkm_example_exit);
