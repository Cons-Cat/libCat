// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::read_vector(nix::FileDescriptor const file_descriptor,
                      cat::Span<nix::IoVector> const& vectors)
    -> Result<ssize> {
    return nix::syscall3(19, file_descriptor, vectors.p_data(), vectors.size());
}
