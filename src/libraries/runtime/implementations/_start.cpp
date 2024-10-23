#include <cat/cpuid>
#include <cat/runtime>
#include <cat/string>

// Attributes on this prototype would have no effect.
auto
main(...) -> int;  // NOLINT Without K&R, `...` is the only way to do this.

namespace {

extern "C" {
#ifndef NO_ARGC_ARGV
[[noreturn, gnu::used, gnu::no_stack_protector, gnu::no_sanitize_address]]
void
call_main_args(int argc, char** p_argv) {
   // The stack pointer must be aligned to prevent SIMD segfaults.
   cat::align_stack_pointer_32();
   // Initialize `__cpu_model` and `__cpu_features2` for later use.
   x64::detail::__cpu_indicator_init();
   cat::exit(main(argc, p_argv));
}
#else
[[noreturn, gnu::no_stack_protector, gnu::no_sanitize_address]]
void
call_main_noargs() {
   // The stack pointer must be aligned to prevent SIMD segfaults.
   cat::align_stack_pointer_32();
   // Initialize `__cpu_model` and `__cpu_features2` for later use.
   x64::detail::__cpu_indicator_init();
   cat::exit(main());
}
#endif
}

using constructor_fn = void (*const)();
extern "C" {
extern constructor_fn __init_array_start[];  // NOLINT
extern constructor_fn __init_array_end[];    // NOLINT
}

}  // namespace

// clang-format off
extern "C"
[[gnu::used
#ifndef NO_ARGC_ARGV
   // If arguments are loaded, this must be `naked` to prevent pushing `%rbp`
   // first, which breaks argument loading. I've tried other solutions, but
   // none worked yet.
   , gnu::naked
#endif
]]
   // clang-format on
   void
   cat::detail::_start() {
   // `NO_ARGC_ARGV` can be defined in a build system to skip argument loading.
#ifndef NO_ARGC_ARGV
   asm(R"(.att_syntax prefix ; # rmsbolt requires this. Try `-masm=att`
          pop %rdi        # Load `int4 argc`.
          mov %rsp, %rsi  # Load `char* argv[]`.
          call call_main_args
       )");
#else
   [[clang::always_inline]] call_main_noargs();
#endif
}
