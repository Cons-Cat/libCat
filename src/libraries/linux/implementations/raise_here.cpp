#include <cat/linux>

auto
nix::raise_here(signal signal) noexcept(!__has_feature(cxx_exceptions))
   -> scaredy_nix<void> {
   return raise(signal, sys_getpid());
}
