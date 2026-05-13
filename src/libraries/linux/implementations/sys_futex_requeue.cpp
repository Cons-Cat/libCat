#include <cat/linux>

auto
nix::sys_futex_requeue(cat::span<futex_waitv const> waiters, cat::uint4 flags,
                       cat::int4 nr_wake, cat::int4 nr_requeue)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(456, cat::unconst(waiters.data()),
                                          flags, nr_wake, nr_requeue);
}
