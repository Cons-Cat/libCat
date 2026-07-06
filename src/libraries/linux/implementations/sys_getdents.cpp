#include <cat/linux>

auto
nix::sys_getdents(file_descriptor file_descriptor, cat::span<char> buffer)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      78, file_descriptor, buffer.data(), buffer.size()
   );
}
