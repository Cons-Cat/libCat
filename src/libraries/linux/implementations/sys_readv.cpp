#include <cat/linux>

auto nix::sys_readv(nix::FileDescriptor const file_descriptor,
                    cat::Span<nix::IoVector> const& vectors)
    -> nix::ScaredyLinux<ssize> {
    return nix::syscall<ssize>(19, file_descriptor, vectors.p_data(),
                               vectors.size());
}
