#include <cat/linux>

auto
nix::sys_futex_waitv(cat::span<futex_waitv const> waiters,
                     futex_waitv_call_flags call_flags,
                     futex_timespec const* _Nullable p_timeout,
                     cat::int4 clock_id) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(
      449, cat::unconst(waiters.data()), waiters.size(),
      static_cast<unsigned int>(call_flags), p_timeout, clock_id);
}
