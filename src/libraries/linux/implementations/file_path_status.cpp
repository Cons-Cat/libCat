// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::file_path_status(String const& file_path) -> Result<nix::FileStatus> {
    nix::FileStatus status;
    return static_cast<Result<nix::FileStatus>>(
               nix::syscall2(4, file_path.p_data(), &status))
        .and_then([&](FileStatus) {
            return status;
        });
    ;
}
