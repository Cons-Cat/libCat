#include <cat/cpuid>
#include <cat/runtime>
#include <cat/string>

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

// `CAT_NO_STATIC_CONSTRUCTORS` lets a target opt out of walking `.init_array`.
// The bounds are linker-defined symbols, so LLVM cannot prove the loop is
// empty even under LTO. Targets that promise no global constructors (e.g. the
// `hello` example) define it to drop the call entirely.
#ifndef CAT_NO_STATIC_CONSTRUCTORS
using constructor_fn = void (*const)();
extern "C" {
extern constructor_fn __init_array_start[];
extern constructor_fn __init_array_end[];

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
#ifndef NO_ARGC_ARGV
[[noreturn, gnu::used, gnu::no_stack_protector, gnu::no_sanitize_address,
  gnu::force_align_arg_pointer]]
void
call_main_args(int argc, char* const* pp_argv) {
#ifndef CAT_NO_CPUID
   // Initialize `__cpu_model` and `__cpu_features2` for later use.
   x64::detail::__cpu_indicator_init();
#endif
#ifndef CAT_NO_STATIC_CONSTRUCTORS
   call_static_constructors();
#endif
   // `[[clang::always_inline]]` overrides LLVM's default bias against
   // inlining `main` (preserved for atexit ordering / debugger step-in
   // semantics) at this specific call site, so `main`'s body folds into
   // `_start`. The same attribute also forces `cat::exit` to inline,
   // matching what LTO already does today for that call.
   [[clang::always_inline]] cat::exit(main(argc, pp_argv));
}
#else
[[noreturn, gnu::always_inline, gnu::no_stack_protector,
  gnu::no_sanitize_address]]
inline void
call_main_noargs() {
#ifndef CAT_NO_CPUID
   // Initialize `__cpu_model` and `__cpu_features2` for later use.
   x64::detail::__cpu_indicator_init();
#endif
#ifndef CAT_NO_STATIC_CONSTRUCTORS
   call_static_constructors();
#endif
   [[clang::always_inline]] cat::exit(main());
}
#endif
}

}  // namespace

extern "C" [[gnu::used, gnu::no_stack_protector]]
#ifndef NO_ARGC_ARGV
// If arguments are loaded, this must be `naked` to prevent pushing `%rbp`
// first, which breaks argument loading. I've tried other solutions, but none
// worked yet.
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
          call call_main_args
       )");
#else
   [[clang::always_inline]] call_main_noargs();
#endif
}
