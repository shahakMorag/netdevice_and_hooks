#include <linux/ftrace.h>
#include <linux/module.h>
#include <linux/dirent.h>
#include <linux/kallsyms.h>
#include <linux/netdevice.h>

#include "common.h"
#include "hook_function.h"

/*
 * It’s a pointer to the original system call handler execve().
 * It can be called from the wrapper. It’s extremely important to keep the function signature
 * without any changes: the order, types of arguments, returned value,
 * and ABI specifier(pay attention to "asmlinkage").
 */
static struct sk_buff *(*real___netdev_alloc_skb)(struct net_device *dev, unsigned int length,
				   gfp_t gfp_mask);

/*
 * This function will be called instead of the hooked one. Its arguments are
 * the arguments of the original function. Its return value will be passed on to
 * the calling function. This function can execute arbitrary code before, after,
 * or instead of the original function.
 */
static struct sk_buff * fh___netdev_alloc_skb(struct net_device *dev, unsigned int length,
				   gfp_t gfp_mask) {
    struct sk_buff * ret;

    printk("__netdev_alloc_skb() called: dev=%p length=%u gfp_mask=%u\n",
            dev, length, gfp_mask);

    ret = real___netdev_alloc_skb(dev, length, gfp_mask);

    printk("__netdev_alloc_skb() returns: %p\n", ret);

    return ret;
}

/**
 * struct ftrace_hook    describes the hooked function
 *
 * @name:                the name of the hooked function
 *
 * @function:            the address of the wrapper function that will be called instead
 *                       of the hooked function
 *
 * @original:            a pointer to the place where the address
 *                       of the hooked function should be stored, filled out during installation
 *                       of the hook
 *
 * @address:             the address of the hooked function, filled out during installation
 *                       of the hook
 *
 * @ops:                 ftrace service information, initialized by zeros;
 *                       initialization is finished during installation of the hook
 */
struct ftrace_hook {
        const char *name;
        void *function;
        void *original;

        unsigned long address;
        struct ftrace_ops ops;
};

#define HOOK(_name, _function, _original)            \
{                                                    \
    .name = (_name),                                 \
    .function = (_function),                         \
    .original = (_original),                         \
}

static struct ftrace_hook hooked_functions[] = {
        HOOK("__netdev_alloc_skb", fh___netdev_alloc_skb, &real___netdev_alloc_skb),
};

unsigned long (*my_lookup_name)(const char * name) = (void *)0xc008704c;

static int resolve_hook_address(struct ftrace_hook *ftrace_hook) {
    ftrace_hook->address = my_lookup_name(ftrace_hook->name);

    if(!ftrace_hook->address) {
        printk("unresolved symbol: %s\n", ftrace_hook->name);
        return -ENOENT;
    }

    *((unsigned long*) ftrace_hook->original) = ftrace_hook->address;

    return 0;
}

static void notrace fh_ftrace_thunk(unsigned long ip, unsigned long parent_ip,
                struct ftrace_ops *ops, struct ftrace_regs *regs) {
    struct ftrace_hook *hook = container_of(ops, struct ftrace_hook, ops);
    /* Skip the function calls from the current module. */
    if(!within_module(parent_ip, THIS_MODULE))
            regs->regs.uregs[15] =(unsigned long) hook->function;
}

int fh_install_hook(struct ftrace_hook *hook) {
    int err = resolve_hook_address(hook);
    if (err)
        return err;

    hook->ops.func = fh_ftrace_thunk;
    hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS;
                    // | FTRACE_OPS_FL_IPMODIFY;

    // err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
    // if(err) {
    //     printk("ftrace_set_filter_ip() failed: %d\n", err);
    //     return err;
    // }

    err = register_ftrace_function(&hook->ops);
    if(err) {
        printk("register_ftrace_function() failed: %d\n", err);

        // /* Don’t forget to turn off ftrace in case of an error. */
        // ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);

        return err;
    }

    return 0;
}

void fh_remove_hook(struct ftrace_hook *ftrace_hook) {
    int err = unregister_ftrace_function(&ftrace_hook->ops);
    if(err) {
        printk("unregister_ftrace_function() failed: %d\n", err);
    }

    // err = ftrace_set_filter_ip(&ftrace_hook->ops, ftrace_hook->address, 1, 0);
    // if(err) {
    //     printk("ftrace_set_filter_ip() failed: %d\n", err);
    // }
}

bool hook_function_init(void) {
    int i = 0;
    for (; i < sizeof(hooked_functions) / sizeof(*hooked_functions); ++i) {
        if (0 != fh_install_hook(hooked_functions + i)) {
            --i;
            goto remove_previous_hooks;
        }
    }

    return true;

remove_previous_hooks:
    for (; i >= 0; --i) {
        fh_remove_hook(hooked_functions + i);
    }

    return false;
}

void hook_function_exit(void) {
    int i = (sizeof(hooked_functions) / sizeof(*hooked_functions)) - 1;
    for (; i >= 0; --i) {
        fh_remove_hook(hooked_functions + i);
    }
}