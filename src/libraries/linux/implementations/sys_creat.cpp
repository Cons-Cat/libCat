// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <cat/linux>

auto nix::sys_creat(char const* p_file_path, nix::OpenMode file_mode)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::sys_open(p_file_path, file_mode,
                         nix::OpenFlags::create | nix::OpenFlags::truncate |
                             nix::OpenMode::read_write);
}
