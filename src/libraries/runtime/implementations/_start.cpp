#include <cat/cpuid>
#ifndef CAT_NO_VDSO
#include <cat/detail/vdso.hpp>
#endif
#include <cat/linux>
#include <cat/runtime>

// We could use a single K&R `main(...)` but that linkage causes trouble for
// LTO, so we specify both of these forms.
#ifndef CAT_NO_ARGC_ARGV
auto
main(int argc, char* const* pp_argv) -> int;
#else
auto
main() -> int;
#endif

namespace {

#ifndef CAT_NO_STATIC_CONSTRUCTORS
using constructor_fn = void (*_Nonnull const)();
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

[[clang::always_inline]]
inline void
detect_syscall_support() {
   nix::kernel_version const version = nix::get_kernel_version();

   // For most syscalls, we assume it is available if the kernel version is
   // at least their minimum required version.
   // TODO: Can we generalize a data type for versions?
   nix::detail::has_sys_cachestat_cache =
      version >= nix::kernel_version{.major = 6, .minor = 5};
   nix::detail::has_sys_fchmodat2_cache =
      version >= nix::kernel_version{.major = 6, .minor = 6};
   // TODO: Shadow stack availability may need deeper introspection.
   nix::detail::has_sys_map_shadow_stack_cache =
      version >= nix::kernel_version{.major = 6, .minor = 6};
   nix::detail::has_sys_futex_wake_cache =
      version >= nix::kernel_version{.major = 6, .minor = 7};
   nix::detail::has_sys_futex_wait_cache =
      version >= nix::kernel_version{.major = 6, .minor = 7};
   nix::detail::has_sys_futex_requeue_cache =
      version >= nix::kernel_version{.major = 6, .minor = 7};
   nix::detail::has_sys_mseal_cache =
      version >= nix::kernel_version{.major = 6, .minor = 10};

   // Base `io_uring` is available from Linux 5.1, but distributions often
   // disable it either via `/proc/sys/kernel/io_uring_disabled` or by compiling
   // their kernels without it.
   //
   // We make a trivial `io_uring_setup` syscall, and if it succeeds, this
   // feature is available.
   nix::io_uring_params params{};
   cat::scaredy result = nix::syscall<nix::file_descriptor>(425, 1u, &params);
   if (result.has_value()) {
      auto _ = nix::sys_close(result.value());
      nix::detail::has_sys_io_uring_cache = true;
   } else {
      // If the runtime is disabled, we get `nix::linux_error::nosys`. If it
      // wasn't compiled in at all, we get `nix::linux_error::perm`.
      nix::detail::has_sys_io_uring_cache = false;
   }
}

#ifndef CAT_NO_VDSO
constexpr cat::uword at_sysinfo_ehdr = 33u;

void
init_vdso(cat::uword const* _Nonnull p_stack) {
   cat::uword const argc = p_stack[0];
   cat::uword const* p_auxiliary = p_stack + argc + 2;
   while (*p_auxiliary != 0) {
      ++p_auxiliary;
   }
   ++p_auxiliary;

   while (p_auxiliary[0] != 0) {
      if (p_auxiliary[0] == at_sysinfo_ehdr) {
         nix::detail::initialize_vdso(
            __builtin_bit_cast(void const*, p_auxiliary[1])
         );
         return;
      }
      p_auxiliary += 2;
   }
}
#endif

extern "C" {
[[noreturn, gnu::no_stack_protector, gnu::no_sanitize_address]]
#if defined(CAT_NO_ARGC_ARGV) && defined(CAT_NO_VDSO)
void
call_main() {
#else
[[gnu::used]]
void
call_main([[maybe_unused]] cat::uword const* _Nonnull p_stack) {
#ifndef CAT_NO_VDSO
   init_vdso(p_stack);
#endif
#endif
#ifndef CAT_NO_CPUID
   // Initialize `__cpu_model` and `__cpu_features2` for later use.
   x64::detail::__cpu_indicator_init();
#endif
#ifndef CAT_NO_SYSCALL_PROBES
   // Initialize the `nix::has_sys_*` flags for later use. Define
   // `CAT_NO_SYSCALL_PROBES` for binaries that never query the
   // `has_sys_*` family, to skip the two startup syscalls
   // (`sys_uname` + `sys_io_uring_setup`).
   detect_syscall_support();
#endif
#if defined(CAT_STATIC_LINKED) \
   && (!defined(CAT_THREAD_LOCAL_SIZE) || (CAT_THREAD_LOCAL_SIZE) != 0)
   // Set up `%fs` so the parent process can access `thread_local` values. Must
   // run before `call_static_constructors` because a constructor body
   // could touch a `thread_local`. The buffer is deliberately leaked
   // (kernel reclaims at `_exit`). Only emitted under static, non-PIE
   // links where no dynamic loader has set `%fs` for us first.
   // `CAT_STATIC_LINKED` is set by the top-level `CMakeLists.txt`.
   nix::detail::init_parent_process_tls();
#endif
#ifndef CAT_NO_STATIC_CONSTRUCTORS
   call_static_constructors();
#endif
#ifdef CAT_NO_ARGC_ARGV
   [[clang::always_inline]] cat::exit(main());
#else
   int const argc = static_cast<int>(p_stack[0]);
   auto const* const pp_argv = __builtin_bit_cast(char* const*, p_stack + 1);
   [[clang::always_inline]] cat::exit(main(argc, pp_argv));
#endif
}
}  // extern "C"

}  // namespace

// The kernel stack is required for argv and for the vDSO auxv.
#if defined(CAT_NO_ARGC_ARGV) && defined(CAT_NO_VDSO)
extern "C" [[gnu::used, gnu::no_stack_protector]]
void
cat::detail::_start() {
   [[clang::always_inline]] call_main();
}
#else
// This can be inlined by BOLT, but not by Clang LTO.
extern "C" [[gnu::used, gnu::no_stack_protector, gnu::naked]]
void
cat::detail::_start() {
   asm(R"(.att_syntax prefix ; # rmsbolt requires this. Try `-masm=att`
          mov %rsp, %rdi  # Preserve the kernel initial stack.
          call call_main
       )");
}
#endif
