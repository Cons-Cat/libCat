#include <cat/runtime>

// Attributes on this prototype would have no effect.
auto
main(...) -> int;  // NOLINT Without K&R, `...` is the only way to do this.

namespace {

[[noreturn,
// `NOINLINE_MAIN` can be defined in the build system to prevent inlining
// `main()` in `_start()`.
#ifndef NOINLINE_MAIN
  gnu::noinline
#endif
]]
void
call_main() {
   // These `register` variables must be uninitialized to load from the stack.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
   [[clang::uninitialized]]
   register int argc asm("rdi");

   [[clang::uninitialized]]
   register char** p_argv asm("rsi");

   cat::exit(main(argc, p_argv));
   __builtin_unreachable();
#pragma clang diagnostic pop
}

}  // namespace

extern "C" void
cat::_start() {
   // `NO_ARGC_ARGV` can be defined in a build system to skip argument loading.
#ifndef NO_ARGC_ARGV
   asm(R"(pop %rdi        # Load `int4 argc`.
           mov %rsp, %rsi  # Load `char* argv[]`.
       )");
#endif

   // The stack pointer must be aligned to prevent SIMD segfaults even in
   // surprisingly simple programs with GCC 12.
   align_stack_pointer_32();

   // `main()` must be wrapped by a function to conditionally prevent inlining.
   call_main();
   __builtin_unreachable();
}
