#include <cat/linux>

auto
nix::sys_getcwd(char* _Nonnull p_buffer, cat::uword length)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<cat::idx>(79, p_buffer, length);
}
