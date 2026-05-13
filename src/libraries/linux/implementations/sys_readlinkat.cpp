#include <cat/linux>

auto
nix::sys_readlinkat(file_descriptor dirfd, char const* p_file_path,
                    char* p_buffer, cat::uword buffer_length)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<cat::idx>(267, dirfd, p_file_path, p_buffer,
                                 buffer_length);
}
