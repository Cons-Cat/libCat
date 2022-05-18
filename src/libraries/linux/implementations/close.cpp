#include <cat/linux>

auto nix::close(nix::FileDescriptor const object) -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(3, object);
}
