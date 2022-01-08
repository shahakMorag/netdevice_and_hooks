#include <linux/skbuff.h>
#include <linux/kthread.h>

#include "push_packet_to_interface.h"

#define SLEEP_BETWEEN_PACKETS (1000)

const unsigned char BEACON_PACKET[] = { 0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0c, 0x41, 0x82, 0xb2, 0x55, 0x00, 0x0c, 0x41, 0x82, 0xb2, 0x55, 0x80, 0xf8, 0x8e, 0x11, 0xd8, 0x1b, 0x01, 0x00, 0x00, 0x00, 0x64, 0x00, 0x11, 0x04, 0x00, 0x07, 0x43, 0x6f, 0x68, 0x65, 0x72, 0x65, 0x72, 0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 0x01, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x2a, 0x01, 0x02, 0x2f, 0x01, 0x02, 0x30, 0x18, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x02, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x02, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00, 0x32, 0x04, 0x0c, 0x12, 0x18, 0x60, 0xdd, 0x06, 0x00, 0x10, 0x18, 0x02, 0x00, 0x04, 0xdd, 0x1c, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00, 0x00, 0x50, 0xf2, 0x02, 0x02, 0x00, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x00, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x00, 0x28, 0x9f, 0xa1, 0xff };

static int push_packet_to_interface(void * data) {
    struct net_device * net_device = data;
	if(NULL == net_device) {
		goto cleanup;
	}

	while (!kthread_should_stop()) {
		struct sk_buff * skb = netdev_alloc_skb(net_device, sizeof(BEACON_PACKET));
		if (NULL == skb) {
			printk("failed to allocate skb");
			goto cleanup;
		}

		memcpy(skb_put(skb, sizeof(BEACON_PACKET)), BEACON_PACKET, sizeof(BEACON_PACKET));

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

bool push_packet_to_interface_init(struct task_struct ** push_packet_kthread, struct net_device * device) {
    static int kthread_created = 0;
    if (NULL == push_packet_kthread || NULL == device) {
        return false;
    }

    *push_packet_kthread = kthread_run(push_packet_to_interface, device, "push_packets_%d", kthread_created++);
    return NULL != *push_packet_kthread;
}

void push_packet_to_interface_exit(struct task_struct ** push_packet_kthread) {
    if (NULL == push_packet_kthread || NULL == *push_packet_kthread) {
        return;
    }

    kthread_stop(*push_packet_kthread);
}