#include <cat/linux>

auto
nix::sys_uname(utsname& out) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(63, &out);
}
