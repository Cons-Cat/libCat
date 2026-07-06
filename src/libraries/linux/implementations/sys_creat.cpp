// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <cat/linux>

auto
nix::sys_creat(cat::zstr_view file_path, open_mode file_mode)
   -> nix::scaredy_nix<file_descriptor> {
   return nix::sys_open(
      file_path, file_mode,
      open_flags::create
         | nix::open_flags::truncate
         | static_cast<nix::open_flags>(
            cat::to_underlying(nix::open_mode::read_write)
         )
   );
}
