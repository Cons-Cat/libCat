#include <cat/linux>

auto
nix::sys_futex_wake(
   futex_word& uaddr, cat::uword mask, cat::int4 nr, cat::uint4 flags
) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(454, &uaddr.m_value, mask, nr, flags);
}
