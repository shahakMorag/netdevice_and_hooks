#include "common.h"
#include "network_interface.h"

#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>


static netdev_tx_t my_dev_start_xmit(struct sk_buff *skb, struct net_device *dev);

struct net_device_ops ndo = {
	.ndo_start_xmit = &my_dev_start_xmit,
};


static netdev_tx_t my_dev_start_xmit(struct sk_buff *skb, struct net_device *dev) {
	kfree_skb(skb);

	return NETDEV_TX_OK;
}

static void netdevice_setup(struct net_device * dev) {
	for (int j = 0; j < ETH_ALEN; ++j) {
		dev->dev_addr[j] = (char)j;
	}

	ether_setup(dev);
	dev->netdev_ops = &ndo;
	dev->type = ARPHRD_IEEE80211;
}

bool add_netdevice(struct net_device ** dev) {
    if (NULL == dev) {
        return false;
    }

	*dev = alloc_netdev(0, "bb%d", NET_NAME_UNKNOWN, netdevice_setup);
    struct net_device * allocated_net_device = *dev;
	if (NULL == allocated_net_device) {
		printk("allocated_net_device alloc failed!\n");
		return false;
	}

	if (0 > dev_alloc_name(allocated_net_device, allocated_net_device->name)) {
		printk("dev_alloc_name failed!\n");
		goto cleaup;
	}

	if (0 > register_netdev(allocated_net_device)) {
		printk("failed to register net device\n");
		goto cleaup;
	}

	rtnl_lock();
	dev_open(allocated_net_device, NULL);
	rtnl_unlock();

	return true;

cleaup:
	free_netdev(allocated_net_device);
	*dev = NULL;
	return false;
}

void remove_netdevice(struct net_device ** dev) {
	if (NULL != dev && NULL != *dev) {
		unregister_netdev(*dev);
		free_netdev(*dev);
        *dev = NULL;
	}
}