#include <cat/linux>

auto
nix::sys_readlink(cat::zstr_view file_path, cat::span<char> buffer)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<cat::idx>(89, file_path.data(), buffer.data(),
                                 buffer.size());
}
