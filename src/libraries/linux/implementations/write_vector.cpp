// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::write_vector(nix::FileDescriptor const file_descriptor,
                       cat::Span<nix::IoVector> const& vectors)
    -> Result<ssize> {
    return nix::syscall3(20, file_descriptor, vectors.p_data(), vectors.size());
}
