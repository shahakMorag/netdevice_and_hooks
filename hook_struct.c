/*
My hooks write to the three opcodes in the start of the function the hook stub.
The hook stub jumps to the replacement function without linking.
Therefore the function must have the same signature as the original function.
When we want to call the original function We use function pointer that points
into buffer that contains the first three opcodes from the original function
and then jumps to the rest of the function.
When using this hooking infrastructure you need to make sure that no caller use r9 for any purpose.
*/
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/cacheflush.h>

#include "common.h"
#include "hook_struct.h"

#define HOOK_STUB_ADDRESS_OFFSET (1)
#define CALL_ORIGINAL_ADDRESS_OFFSET (4)

const unsigned long CALL_ORIGINAL_FUNCTION_EXAMPLE[] = {
    0x0, // original code from the function
    0x0, // original code from the function
    0x0, // original code from the function
    0xe51f9004, // ldr r9
    0x0, // address to jump to
    0xe1a0f009 // mov r9 to pc
};

const unsigned long HOOK_STUB_EXAMPLE[] = {
    0xe51f9004, // ldr r9
    0x0, // address to jump to
    0xe1a0f009 // mov r9 to pc
};

static void build_call_original_code(struct hook * hook, unsigned long original_code_address) {
    // code from original function
    unsigned long * original_code = (void *)original_code_address;
    for (int i = 0; i < CALL_ORIGINAL_ADDRESS_OFFSET - 1; ++i) {
        hook->call_original_function[i] = original_code[i];
    }

    // code to jump to the rest of the function
    hook->call_original_function[CALL_ORIGINAL_ADDRESS_OFFSET - 1] = CALL_ORIGINAL_FUNCTION_EXAMPLE[CALL_ORIGINAL_ADDRESS_OFFSET - 1];
    hook->call_original_function[CALL_ORIGINAL_ADDRESS_OFFSET] = original_code_address + sizeof(HOOK_STUB_EXAMPLE);
    hook->call_original_function[CALL_ORIGINAL_ADDRESS_OFFSET + 1] = CALL_ORIGINAL_FUNCTION_EXAMPLE[CALL_ORIGINAL_ADDRESS_OFFSET + 1];
}

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

    build_call_original_code(res, address);

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