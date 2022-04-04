// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::file_status(nix::FileDescriptor file_descriptor)
    -> Result<nix::FileStatus> {
    nix::FileStatus status;
    return static_cast<Result<FileStatus>>(
               nix::syscall2(5, file_descriptor, &status))
        .and_then([&](FileStatus) {
            return status;
        });
}
