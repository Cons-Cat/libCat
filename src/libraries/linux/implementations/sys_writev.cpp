#include <cat/linux>

auto nix::sys_writev(nix::FileDescriptor file_descriptor,
                     cat::Span<nix::IoVector> const& vectors)
    -> nix::ScaredyLinux<ssize> {
    return nix::syscall<ssize>(20, file_descriptor, vectors.data(),
                               vectors.size());
}
