#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/cacheflush.h>

#include "common.h"
#include "hook_struct.h"

#define CALL_ORIGINAL_ADDRESS_OFFSET (4)

const unsigned long CALL_ORIGINAL_FUNCTION_EXAMPLE[] = {
    0xe1a0c00d, // mov r12, sp
    0xe92dd8f0, // push {r4, r5, r6, r7, r11, r12, lr, pc}
    0xe24cb004, // sub r11, r12, #4
    0xe51f9004, // ldr r9
    0x0, // address to jump to
    0xe1a0f009 // mov r9 to pc
};

#define HOOK_STUB_ADDRESS_OFFSET (1)

const unsigned long HOOK_STUB_EXAMPLE[] = {
    0xe51f9004, // ldr r9
    0x0, // address to jump to
    0xe1a0f009 // mov r9 to pc
};

struct hook * create_hook(unsigned long address, unsigned long replacement) {
    if (0 == address) {
        return NULL;
    }

    struct hook * res = kmalloc(sizeof(*res), GFP_KERNEL);
    if (NULL == res) {
        printk("failed to allocate hook!\n");
        return NULL;
    }

    res->function_address = address;

    memcpy(res->hook_stub, HOOK_STUB_EXAMPLE, sizeof(HOOK_STUB_EXAMPLE));

    unsigned long * original_code = (void *)address;
    for (int i = 0; i < CALL_ORIGINAL_ADDRESS_OFFSET - 1; ++i) {
        res->call_original_function[i] = original_code[i];
    }

    res->call_original_function[CALL_ORIGINAL_ADDRESS_OFFSET - 1] = CALL_ORIGINAL_FUNCTION_EXAMPLE[CALL_ORIGINAL_ADDRESS_OFFSET - 1];
    res->call_original_function[CALL_ORIGINAL_ADDRESS_OFFSET] = address + sizeof(HOOK_STUB_EXAMPLE);
    res->call_original_function[CALL_ORIGINAL_ADDRESS_OFFSET + 1] = CALL_ORIGINAL_FUNCTION_EXAMPLE[CALL_ORIGINAL_ADDRESS_OFFSET + 1];

    res->hook_stub[HOOK_STUB_ADDRESS_OFFSET] = replacement;

    return res;
}

unsigned long (*my_lookup_name)(const char * name) = (void *)0xc008704c;

struct hook * create_hook_name(char * symbol, unsigned long replacement) {
    if (NULL == symbol) {
        return NULL;
    }

    unsigned long address = my_lookup_name(symbol);
    return create_hook(address, replacement);
}

void install_hook(struct hook * hook) {
    if (NULL == hook) {
        return;
    }

    unsigned long flags = 0;
    local_irq_save(flags);

    memcpy((void *)hook->function_address, hook->hook_stub, sizeof(HOOK_STUB_EXAMPLE));
    flush_icache_range(hook->function_address, hook->function_address + sizeof(HOOK_STUB_EXAMPLE));

    local_irq_restore(flags);
}

void remove_hook(struct hook * hook) {
    if (NULL == hook) {
        return;
    }

    unsigned long flags = 0;
    local_irq_save(flags);

    memcpy((void *)hook->function_address, hook->call_original_function, sizeof(HOOK_STUB_EXAMPLE));
    flush_icache_range(hook->function_address, hook->function_address + sizeof(HOOK_STUB_EXAMPLE));

    local_irq_restore(flags);
}