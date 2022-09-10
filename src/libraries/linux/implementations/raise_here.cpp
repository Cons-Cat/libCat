#include <cat/linux>

auto nix::raise_here(Signal signal) -> ScaredyLinux<void> {
    return raise(signal, sys_getpid());
}
