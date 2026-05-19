#include <cat/linux>

auto
nix::sys_pread64(file_descriptor file_descriptor, void* _Nonnull p_buffer,
                 cat::iword length, cat::iword offset)
   -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(17, file_descriptor, p_buffer,
                                            length, offset);
}
