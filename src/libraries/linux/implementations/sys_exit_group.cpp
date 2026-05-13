#include <cat/linux>

[[noreturn]]
void
nix::sys_exit_group(cat::int4 status) {
   for (;;) {
      // https://filippo.io/linux-syscall-table/
      nix::syscall1_volatile(231, status);
   }
}
