#include <cat/linux>

auto
nix::sys_getgroups(cat::span<group_id> list) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<cat::idx>(115, list.size(), list.data());
}
