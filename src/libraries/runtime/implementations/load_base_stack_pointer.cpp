#include <cat/runtime>

auto cat::load_base_stack_pointer() -> void* {
    void* rbp;
    asm("mov %%rbp, %[rbp]"
        : [rbp] "=r"(rbp));
    return rbp;
}
