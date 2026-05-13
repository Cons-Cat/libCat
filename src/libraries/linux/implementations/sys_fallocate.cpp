#include <cat/linux>

auto
nix::sys_fallocate(file_descriptor file_descriptor, fallocate_flags mode,
                   cat::iword offset, cat::iword length)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(285, file_descriptor, mode, offset,
                                      length);
}
