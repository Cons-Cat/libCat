#include <cat/linux>

// `read()` transmits a number of bytes into a file descriptor.
auto
nix::sys_read(file_descriptor file_descriptor, cat::span<char> buffer)
   -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(
      0, file_descriptor, buffer.data(), buffer.size()
   );
}
