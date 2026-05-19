#include <cat/runtime>

[[gnu::returns_nonnull]]
auto
cat::load_base_stack_pointer() -> void* _Nonnull {
   void* _Nonnull p_stack_base;
   asm("mov %%rbp, %[rbp]"
       : [rbp] "=r"(p_stack_base));
   return p_stack_base;
}
