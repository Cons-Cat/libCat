// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::file_descriptor_status(nix::FileDescriptor const file_descriptor)
    -> nix::ScaredyLinux<nix::FileStatus> {
    nix::FileStatus status;
    auto result = nix::syscall<void>(5, file_descriptor, &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<nix::LinuxError>();
}
