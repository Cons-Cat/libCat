#include <cat/linux>

auto nix::sys_unlink(char const* p_path_name) -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(87, p_path_name);
}
