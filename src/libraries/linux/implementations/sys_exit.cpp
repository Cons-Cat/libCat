#include <cat/linux>

// `sys_exit` does not require asan and instrumenting it causes edge-case
// false-positives in `nix::process`.
[[noreturn, gnu::no_sanitize_address]]
void
nix::sys_exit(cat::int4 status) {
   for (;;) {
      // https://filippo.io/linux-syscall-table/
      nix::syscall1_volatile(60, status);
   }
}
