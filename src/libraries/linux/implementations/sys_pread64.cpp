#include <cat/linux>

auto
nix::sys_pread64(
   file_descriptor file_descriptor, cat::span<char> buffer, cat::iword offset
) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(
      17, file_descriptor, buffer.data(), buffer.size(), offset
   );
}
