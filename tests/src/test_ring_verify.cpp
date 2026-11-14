// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/ring>`. Any `expected-error`
// checks must come last, because Clang stops its `-Wlifetime-safety` pass after
// the first error in a translation unit. Run with `just test safety`.

#include <cat/ring>

using namespace cat::arithmetic;

// Negative cases: the elements outlive their ring.

auto
dangling_ring_data() -> int4* {
   cat::ring<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

void
use_after_scope_ring() {
   int4* p_escaped = nullptr;
   {
      cat::ring<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}
