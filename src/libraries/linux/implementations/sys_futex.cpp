#include <cat/linux>

static_assert(sizeof(nix::futex_timespec) == 16);
static_assert(sizeof(nix::futex_waitv) == 24);
static_assert(sizeof(nix::robust_list_head) == 24);

auto
nix::sys_futex(futex_word& futex, futex_op operation, cat::uint4 value,
               futex_timespec const* _Nullable p_timeout,
               futex_word* _Nullable p_second_futex, cat::uint4 value3)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      202, &futex.m_value, operation.encoded, value, p_timeout,
      p_second_futex != nullptr ? &p_second_futex->m_value : nullptr, value3);
}

auto
nix::sys_futex(futex_word& futex, futex_op operation, cat::uint4 value,
               cat::uint4 value2, futex_word* _Nullable p_second_futex,
               cat::uint4 value3) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      202, &futex.m_value, operation.encoded, value, value2,
      p_second_futex != nullptr ? &p_second_futex->m_value : nullptr, value3);
}
