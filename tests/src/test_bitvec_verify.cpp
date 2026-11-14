// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/bitvec>` and
// `<cat/raii_bitvec>`. Any `expected-error` checks must come last, because
// Clang stops its `-Wlifetime-safety` pass after the first error in a
// translation unit. Run with `just test safety`.

#include <cat/allocator_page>
#include <cat/bitvec>
#include <cat/raii_bitvec>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Negative cases: bits and iterators outlive their bitvec.

auto
dangling_bitvec_begin() -> auto {
   cat::bitvec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_bitvec_data() -> auto {
   cat::bitvec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_bitvec_begin() -> auto {
   cat::raii::bitvec<> local{cat::allocator_dyn(g_page)};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_bitvec_data() -> auto {
   cat::raii::bitvec<> local{cat::allocator_dyn(g_page)};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}
