#include <cat/linux>

auto
nix::sys_getdents(file_descriptor file_descriptor, linux_dirent* p_buffer,
                  cat::uword length) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(78, file_descriptor, p_buffer,
                                          length);
}
