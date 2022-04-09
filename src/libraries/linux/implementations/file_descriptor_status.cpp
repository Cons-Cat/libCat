// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::file_descriptor_status(nix::FileDescriptor const file_descriptor)
    -> Result<nix::FileStatus> {
    nix::FileStatus status;
    return static_cast<Result<FileStatus>>(
               nix::syscall2(5, file_descriptor, &status))
        .and_then([&](FileStatus) {
            return status;
        });
}
