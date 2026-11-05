#include <cat/linux>
#include <cat/runtime>

#if __has_feature(thread_sanitizer)
// TSan's `_exit` interceptor prints its summary and replaces a success code
// with `exitcode` (66 by default) when it reported anything. Raw syscalls skip
// it, so races would not fail a run.
extern "C" [[noreturn]]
void
_exit(cat::int4 exit_code);  // NOLINT(bugprone-reserved-identifier)
#endif

// Terminate the program. Without arguments, this exits with a success code for
// the target operating system.
[[noreturn, gnu::leaf]]
void
cat::exit(iword exit_code) {
#if __has_feature(thread_sanitizer)
   // The kernel only keeps the low byte of an exit code anyway, so this mask
   // makes the narrowing safe.
   _exit(
      narrow_cast<int4>(__builtin_bit_cast(uword, exit_code) & 0xffu).assert()
   );
#else
   asm("syscall"
       :
       : "D"(exit_code.raw), "a"(60)
       :);
   // TODO: Is this still necessary:
#endif
   __builtin_unreachable();  // This elides a `ret` instruction.
}

// TODO: We need `quick_exit` and `terminate` handlers support.

[[noreturn]]
void
cat::terminate() {
   // 134 is the conventional "abort" exit code.
   nix::sys_exit_group(134);
}
