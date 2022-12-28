// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <cat/linux>

auto nix::sys_creat(char const* p_file_path, nix::open_mode file_mode)
    -> nix::scaredy_nix<nix::file_descriptor> {
    return nix::sys_open(p_file_path, file_mode,
                         nix::open_flags::create | nix::open_flags::truncate |
                             nix::open_mode::read_write);
}
