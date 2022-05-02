// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::unlink(char const* p_path_name) -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(87, p_path_name);
}
