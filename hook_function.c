#include <linux/delay.h>
#include <linux/skbuff.h>

#include "hook_struct.h"
#include "hook_function.h"

struct hook * mutex_unlock_hook_struct = NULL;
int (*mutex_unlock_real)(struct mutex * mutex) = NULL;

int mutex_unlock_hook(struct mutex * mutex) {
    // printk("hi from mutex_unlock_hook mutex: %p\n", mutex);

    int res = mutex_unlock_real(mutex);
    // printk("res: %d", res);

    return res;
}

bool hook_function_init(void) {
    mutex_unlock_hook_struct = create_hook_name("mutex_unlock", (unsigned long)mutex_unlock_hook);
    if (NULL == mutex_unlock_hook_struct) {
        return false;
    }

    mutex_unlock_real = (void *)mutex_unlock_hook_struct->call_original_function;

    install_hook(mutex_unlock_hook_struct);

    return true;
}

void hook_function_exit(void) {
    if (NULL == mutex_unlock_hook_struct) {
        return;
    }

    remove_hook(mutex_unlock_hook_struct);

    // wait for all threads and shit to finish before free the hook
    msleep(1000);

    kfree(mutex_unlock_hook_struct);
    mutex_unlock_hook_struct = NULL;
}