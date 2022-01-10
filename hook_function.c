#include <linux/delay.h>
#include <linux/skbuff.h>

#include "hook_struct.h"
#include "hook_function.h"

struct hook * netif_rx_hook_struct = NULL;
int (*netif_rx_real)(struct sk_buff *skb) = NULL;

int netif_rx_hook(struct sk_buff *skb) {
    printk("hi from netif_rx_hook skb: %p\n", skb);

    int res = netif_rx_real(skb);
    printk("res: %d", res);

    return res;
}

bool hook_function_init(void) {
    netif_rx_hook_struct = create_hook_name("netif_rx", (unsigned long)netif_rx_hook);
    if (NULL == netif_rx_hook_struct) {
        return false;
    }

    netif_rx_real = (void *)netif_rx_hook_struct->call_original_function;

    install_hook(netif_rx_hook_struct);

    return true;
}

void hook_function_exit(void) {
    if (NULL == netif_rx_hook_struct) {
        return;
    }

    remove_hook(netif_rx_hook_struct);

    // wait for all threads and shit to finish before free the hook
    msleep(1000);

    kfree(netif_rx_hook_struct);
    netif_rx_hook_struct = NULL;
}