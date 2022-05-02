// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::create_file(char const* p_file_path, nix::OpenMode file_mode)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::open_file(p_file_path, file_mode,
                          nix::OpenFlags::create | nix::OpenFlags::truncate |
                              nix::OpenMode::read_write);
}
