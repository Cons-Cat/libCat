#include <cat/linux>

auto
nix::sys_setgroups(cat::span<group_id const> list) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(116, list.size(), list.data());
}
