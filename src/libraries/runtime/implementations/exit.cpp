#include <cat/linux>
#include <cat/runtime>

// Terminate the program. Without arguments, this exits with a success code for
// the target operating system.
[[noreturn, gnu::leaf]]
void
cat::exit(iword exit_code) {
   asm("syscall"
       :
       : "D"(exit_code.raw), "a"(60)
       :);
   // TODO: Is this still necessary:
   __builtin_unreachable();  // This elides a `ret` instruction.
}

// TODO: We need `quick_exit` and `terminate` handlers support.

[[noreturn]]
void
cat::terminate() {
   // 134 is the conventional "abort" exit code.
   nix::sys_exit_group(134);
}
