// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <cat/linux>

auto
nix::sys_creat(char const* _Nonnull p_file_path, open_mode file_mode)
   -> nix::scaredy_nix<file_descriptor> {
   return nix::sys_open(p_file_path, file_mode,
                        open_flags::create
                           | nix::open_flags::truncate
                           | static_cast<nix::open_flags>(
                              cat::to_underlying(nix::open_mode::read_write)));
}
