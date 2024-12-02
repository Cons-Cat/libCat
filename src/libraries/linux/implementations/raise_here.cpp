#include <cat/linux>

auto
nix::raise_here(signal signal) -> scaredy_nix<void> {
   return raise(signal, sys_getpid());
}
