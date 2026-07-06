#include <cat/linux>

auto
nix::sys_lseek(
   file_descriptor file_descriptor, cat::iword offset, seek_whence whence
) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(8, file_descriptor, offset, whence);
}
