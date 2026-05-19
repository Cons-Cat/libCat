#include <cat/linux>

auto
nix::sys_pwrite64(file_descriptor file_descriptor,
                  void const* _Nonnull p_buffer, cat::iword length,
                  cat::iword offset) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(18, file_descriptor, p_buffer, length,
                                          offset);
}
