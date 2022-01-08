#ifndef __PUSH_PACKET_TO_INTERFACE__
#define __PUSH_PACKET_TO_INTERFACE__

#include <stdbool.h>
#include <linux/netdevice.h>

bool push_packet_to_interface_init(struct task_struct ** push_packet_kthread, struct net_device * data);

void push_packet_to_interface_exit(struct task_struct ** push_packet_kthread);

#endif // !__PUSH_PACKET_TO_INTERFACE__