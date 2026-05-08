#include <cat/cpuid>
#include <cat/linux>
#include <cat/runtime>

// We could use a simple K&R `main(...)` but that linkage causes trouble for
// LTO, so we specify.
#ifndef NO_ARGC_ARGV
auto
main(int argc, char* const* pp_argv) -> int;
#else
auto
main() -> int;
#endif

namespace {

#ifndef CAT_NO_STATIC_CONSTRUCTORS
using constructor_fn = void (*const)();
extern "C" {

extern constructor_fn __init_array_start[];  // NOLINT
extern constructor_fn __init_array_end[];    // NOLINT

void
call_static_constructors() {
   // Execute all function pointers in `__init_array_start`.
   for (constructor_fn const* pp_ctor_func = __init_array_start;
        pp_ctor_func < __init_array_end; ++pp_ctor_func) {
      constructor_fn p_ctor = *pp_ctor_func;
      p_ctor();
   }
}
}
#endif

extern "C" {
[[noreturn, gnu::no_stack_protector, gnu::no_sanitize_address]]
#ifndef NO_ARGC_ARGV
[[gnu::used]]
void
call_main(int argc, char* const* pp_argv)
#else
void
call_main()
#endif
{
#ifndef CAT_NO_CPUID
   // Initialize `__cpu_model` and `__cpu_features2` for later use.
   x64::detail::__cpu_indicator_init();
#endif
#if !defined(CAT_THREAD_LOCAL_SIZE) || (CAT_THREAD_LOCAL_SIZE) != 0
   // Set up `%fs` so the parent process can access `thread_local` values. Must
   // run before `call_static_constructors` because a constructor body
   // could touch a `thread_local`. The buffer is intentionally leaked
   // (kernel reclaims at `_exit`).
   nix::detail::init_parent_process_tls();
#endif
#ifndef CAT_NO_STATIC_CONSTRUCTORS
   call_static_constructors();
#endif
#ifdef NO_ARGC_ARGV
   [[clang::always_inline]] cat::exit(main());
#else
   [[clang::always_inline]] cat::exit(main(argc, pp_argv));
#endif
}
}  // extern "C"

}  // namespace

extern "C" [[gnu::used, gnu::no_stack_protector]]
#ifndef NO_ARGC_ARGV
// If arguments are loaded, this must be `naked` to prevent pushing `%rbp`
// first, which misaligns argument loading.
[[gnu::naked]]
#else
// The kernel hands `_start` a 16-aligned `%rsp` with no return address, so
// the standard `pushq %rbp` prologue would leave `%rbp` 8-aligned and crash
// any inlined SIMD load that addresses locals off `%rbp` (e.g. SIMD spills
// from `__cpu_indicator_init`). `force_align_arg_pointer` emits an alternate
// prologue that realigns the frame to 16 bytes before establishing `%rbp`.
[[gnu::force_align_arg_pointer]]
#endif
void
cat::detail::_start() {
   // `NO_ARGC_ARGV` can defined from a CMake target to skip argument loading.
   // The argument loading version must stay in `asm`. `_start` is `gnu::naked`
   // so the prologue won't push `%rbp` before reading `argc`/`argv` off the
   // kernel-supplied stack.
#ifndef NO_ARGC_ARGV
   asm(R"(.att_syntax prefix ; # rmsbolt requires this. Try `-masm=att`
          pop %rdi        # Load `int4 argc`.
          mov %rsp, %rsi  # Load `char* argv[]`.
          call call_main
       )");
#else
   [[clang::always_inline]] call_main();
#endif
}
