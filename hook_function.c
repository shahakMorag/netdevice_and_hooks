#include <linux/delay.h>
#include <linux/skbuff.h>

#include "hook_struct.h"
#include "hook_function.h"

struct hook * netdev_alloc_skb_hook_struct = NULL;
struct sk_buff * (*netdev_alloc_skb_real)(struct net_device *dev, unsigned int length, gfp_t gfp_mask) = NULL;

struct sk_buff * netdev_alloc_skb_hook(struct net_device *dev, unsigned int length, gfp_t gfp_mask) {
    printk("hi from netdev_alloc_skb_hook dev: %p, length: %u, mask: %u", dev, length, gfp_mask);

    struct sk_buff * res = netdev_alloc_skb_real(dev, length, gfp_mask);
    printk("res: %p", res);

    return res;
}

bool hook_function_init(void) {
    netdev_alloc_skb_hook_struct = create_hook_name("__netdev_alloc_skb", (unsigned long)netdev_alloc_skb_hook);
    if (NULL == netdev_alloc_skb_hook_struct) {
        return false;
    }

    netdev_alloc_skb_real = (void *)netdev_alloc_skb_hook_struct->call_original_function;

    install_hook(netdev_alloc_skb_hook_struct);

    return true;
}

void hook_function_exit(void) {
    if (NULL == netdev_alloc_skb_hook_struct) {
        return;
    }

    remove_hook(netdev_alloc_skb_hook_struct);

    // wait for all threads and shit to finish before free the hook
    msleep(1000);

    kfree(netdev_alloc_skb_hook_struct);
    netdev_alloc_skb_hook_struct = NULL;
}