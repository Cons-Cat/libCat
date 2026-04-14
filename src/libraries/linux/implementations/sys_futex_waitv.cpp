#include <cat/linux>

auto
nix::sys_futex_waitv(cat::span<futex_waitv const> waiters,
                     futex_waitv_call_flags call_flags,
                     futex_timespec const* p_timeout, cat::int4 clock_id)
   -> nix::scaredy_nix<cat::iword> {
   return nix::syscall<cat::iword>(
      449, cat::unconst(waiters.data()), waiters.size(),
      static_cast<unsigned int>(call_flags), p_timeout, clock_id);
}
