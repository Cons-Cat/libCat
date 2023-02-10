#include <cat/linux>

auto nix::sys_writev(nix::file_descriptor file_descriptor,
                     cat::span<nix::io_vector> const& vectors)
    -> nix::scaredy_nix<cat::iword> {
    return nix::syscall<cat::iword>(20, file_descriptor, vectors.data(),
                                    vectors.size());
}
