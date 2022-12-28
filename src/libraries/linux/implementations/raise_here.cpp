#include <cat/linux>

auto nix::raise_here(Signal signal) -> scaredy_nix<void> {
    return raise(signal, sys_getpid());
}
