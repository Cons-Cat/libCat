#include <cat/linux>

auto
nix::sys_io_uring_enter(
   file_descriptor ring, cat::uint4 to_submit, cat::uint4 min_complete,
   io_uring_enter_flags flags, void const* _Nullable p_extended_argument,
   cat::uword extended_argument_size
) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      426, ring, to_submit, min_complete, flags, p_extended_argument,
      extended_argument_size
   );
}
