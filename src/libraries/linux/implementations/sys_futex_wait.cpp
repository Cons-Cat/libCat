#include <cat/linux>

auto
nix::sys_futex_wait(futex_word& uaddr, cat::uword value, cat::uword mask,
                    cat::uint4 flags, futex_timespec const* _Nullable p_timeout,
                    cat::int4 clock_id) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(455, &uaddr.m_value, value, mask, flags,
                                      p_timeout, clock_id);
}
