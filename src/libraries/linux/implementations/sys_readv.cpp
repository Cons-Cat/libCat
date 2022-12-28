#include <cat/linux>

auto nix::sys_readv(nix::file_descriptor file_descriptor,
                    cat::span<nix::io_vector> const& vectors)
    -> nix::scaredy_nix<ssize> {
    return nix::syscall<ssize>(19, file_descriptor, vectors.data(),
                               vectors.size());
}
