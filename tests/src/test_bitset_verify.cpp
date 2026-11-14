// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/bitset>`. Any
// `expected-error` checks must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit. Run
// with `just test safety`.

#include <cat/bitset>

using namespace cat::arithmetic;

// Negative cases: iterators outlive their bitset.

auto
dangling_bitset_begin() -> auto {
   cat::bitset<10u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_bitset_const_begin() -> auto {
   cat::bitset<10u> const local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}
