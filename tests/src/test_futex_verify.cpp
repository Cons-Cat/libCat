// Clang `-verify` checks of the constant argument checks on `nix::futex`. Run
// with `just test safety`.

#include <cat/linux>

using namespace cat::arithmetic;

void
futex_constant_arguments() {
   nix::futex futex_word{};
   auto _ = futex_word.wake(1);
   auto _ = futex_word.wake_bitset(1, nix::futex_bitset::match_any);

   // expected-error@cat/detail/syscalls.hpp:* 2 {{count must be positive!}}
   futex_word.wake(0);
   futex_word.wake(-1);
   futex_word.wake_bitset(0, nix::futex_bitset::match_any);
   futex_word.wake_bitset(-1, nix::futex_bitset::match_any);
}
