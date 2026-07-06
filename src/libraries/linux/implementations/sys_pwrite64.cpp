#include <cat/linux>

auto
nix::sys_pwrite64(
   file_descriptor file_descriptor, cat::str_view buffer, cat::iword offset
) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      18, file_descriptor, buffer.data(), buffer.size(), offset
   );
}
