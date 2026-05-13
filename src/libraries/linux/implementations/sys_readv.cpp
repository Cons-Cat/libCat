#include <cat/linux>

auto
nix::sys_readv(nix::file_descriptor file_descriptor,
               cat::span<nix::io_vector> const& vectors)
   -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(19, file_descriptor, vectors.data(),
                                            vectors.size());
}
