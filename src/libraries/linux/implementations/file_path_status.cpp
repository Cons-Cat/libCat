// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::file_path_status(cat::String const& file_path)
    -> nix::ScaredyLinux<nix::FileStatus> {
    nix::FileStatus status;
    auto result = nix::syscall<void>(4, file_path.p_data(), &status);
    if (result.has_value()) {
        return status;
    }
    return result.error<nix::LinuxError>();
}
