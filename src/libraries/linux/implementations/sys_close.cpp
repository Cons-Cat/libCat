#include <cat/linux>

auto nix::sys_close(nix::file_descriptor object) -> nix::scaredy_nix<void> {
    return nix::syscall<void>(3, object);
}
