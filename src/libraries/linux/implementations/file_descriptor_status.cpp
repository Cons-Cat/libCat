// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::file_descriptor_status(nix::FileDescriptor const file_descriptor)
    -> cat::Scaredy<nix::FileStatus, nix::LinuxError> {
    nix::FileStatus status;
    cat::Scaredy result = nix::syscall<void>(5, file_descriptor, &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<nix::LinuxError>();
}
