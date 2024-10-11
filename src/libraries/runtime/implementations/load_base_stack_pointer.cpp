#include <cat/runtime>

auto
cat::load_base_stack_pointer() -> void* {
   void* p_stack_base;
   asm("mov %%rbp, %[rbp]"
       : [rbp] "=r"(p_stack_base));
   return p_stack_base;
}
