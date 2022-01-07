#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/skbuff.h>
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

#define SLEEP_BETWEEN_PACKETS (1000)
#define UNUSED(a) do { (void)a; } while (0)

struct net_device * g_net_device = NULL;
struct task_struct * g_push_packet_kthread = NULL;

static netdev_tx_t my_dev_start_xmit(struct sk_buff *skb, struct net_device *dev);

const unsigned char BEACON_PACKET[] = { 0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0c, 0x41, 0x82, 0xb2, 0x55, 0x00, 0x0c, 0x41, 0x82, 0xb2, 0x55, 0x80, 0xf8, 0x8e, 0x11, 0xd8, 0x1b, 0x01, 0x00, 0x00, 0x00, 0x64, 0x00, 0x11, 0x04, 0x00, 0x07, 0x43, 0x6f, 0x68, 0x65, 0x72, 0x65, 0x72, 0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 0x01, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x2a, 0x01, 0x02, 0x2f, 0x01, 0x02, 0x30, 0x18, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x02, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x02, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00, 0x32, 0x04, 0x0c, 0x12, 0x18, 0x60, 0xdd, 0x06, 0x00, 0x10, 0x18, 0x02, 0x00, 0x04, 0xdd, 0x1c, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00, 0x00, 0x50, 0xf2, 0x02, 0x02, 0x00, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x00, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x00, 0x28, 0x9f, 0xa1, 0xff };
struct net_device_ops ndo = {
	.ndo_start_xmit = &my_dev_start_xmit,
};


static netdev_tx_t my_dev_start_xmit(struct sk_buff *skb, struct net_device *dev) {
	kfree_skb(skb);

	return NETDEV_TX_OK;
}

int push_packet_to_interface(void * data) {
	UNUSED(data);

	if(NULL == g_net_device) {
		goto cleanup;
	}

	while (!kthread_should_stop()) {
		struct sk_buff * skb = netdev_alloc_skb(g_net_device, sizeof(*BEACON_PACKET));
		if (NULL == skb) {
			printk("failed to allocate skb");
			goto cleanup;
		}

		memcpy(skb_push(skb, sizeof(*BEACON_PACKET)), BEACON_PACKET, sizeof(*BEACON_PACKET));

		skb_reset_mac_header(skb);
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb->pkt_type = PACKET_OTHERHOST;
		skb->protocol = htons(ETH_P_802_2);

		netif_rx(skb);
		msleep_interruptible(SLEEP_BETWEEN_PACKETS);
	}

cleanup:
	while (!kthread_should_stop()) {
		schedule();
	}

	return 0;
}

void netdevice_setup(struct net_device * dev) {
	int j = 0;

	for (; j < ETH_ALEN; ++j) {
		dev->dev_addr[j] = (char)j;
	}

	ether_setup(dev);
	dev->netdev_ops = &ndo;
	dev->type = ARPHRD_IEEE80211;
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

	rtnl_lock();
	dev_open(g_net_device, NULL);
	rtnl_unlock();

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

	g_push_packet_kthread = kthread_run(push_packet_to_interface, NULL, "push_packets");
	if (NULL == g_push_packet_kthread) {
		remove_netdevice();
		return -1;
	}

	return 0;
}
static void __exit lkm_example_exit(void) {
	if (NULL != g_push_packet_kthread) {
		kthread_stop(g_push_packet_kthread);
	}

	remove_netdevice();
}

module_init(lkm_example_init);
module_exit(lkm_example_exit);
