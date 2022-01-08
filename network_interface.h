#ifndef __NETWORK_INTERFACE__
#define __NETWORK_INTERFACE__

#include <linux/netdevice.h>

#include <stdbool.h>

// add wireless netdevice
bool add_netdevice(struct net_device ** dev);

// remove the wireless netdevice
void remove_netdevice(struct net_device ** dev);

#endif // !__NETWORK_INTERFACE__