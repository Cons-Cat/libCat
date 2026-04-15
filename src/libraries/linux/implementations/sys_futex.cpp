#include <cat/linux>

static_assert(sizeof(nix::futex_timespec) == 16);
static_assert(sizeof(nix::futex_waitv) == 24);
static_assert(sizeof(nix::robust_list_head) == 24);

auto
nix::sys_futex(futex_word* p_futex, futex_op const& operation, cat::uint4 value,
               futex_timespec const* p_timeout, futex_word* p_second_futex,
               cat::uint4 value3) -> nix::scaredy_nix<cat::idx> {
   return nix::syscall<cat::idx>(
      202, &p_futex->m_value, operation.encoded, value, p_timeout,
      p_second_futex != nullptr ? &p_second_futex->m_value : nullptr, value3);
}
