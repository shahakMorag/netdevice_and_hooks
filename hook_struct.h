#ifndef __HOOK_STRUCT__
#define __HOOK_STRUCT__

#define HOOK_STUB_SIZE (3)
#define CALL_ORIGINAL_FUNCTION_SIZE (6)

struct hook {
    unsigned long call_original_function[CALL_ORIGINAL_FUNCTION_SIZE];
    unsigned long hook_stub[HOOK_STUB_SIZE];
    unsigned long function_address;
};

struct hook * create_hook(unsigned long address, unsigned long replacement);

struct hook * create_hook_name(char * symbol, unsigned long replacement);

void install_hook(struct hook * hook);

void remove_hook(struct hook * hook);

#endif // !__HOOK_STRUCT__
