// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/vec>`, `<cat/vec_inplace>`,
// and `<cat/raii_vec>`. Any `expected-error` checks must come last, because
// Clang stops its `-Wlifetime-safety` pass after the first error in a
// translation unit. Run with `just test safety`.

#include <cat/allocator_page>
#include <cat/raii_vec>
#include <cat/vec>
#include <cat/vec_inplace>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Positive cases: elements copied out of a local container, and views of a
// container that outlives them.

auto
valid_vec_element_copy() -> int4 {
   cat::vec<int4> local;
   return local[0u];
}

auto
valid_vec_borrow(cat::vec<int4>& caller_vec [[clang::lifetimebound]]) -> int4& {
   return caller_vec[0u];
}

// Negative cases: element references and iterators outlive their container.

auto
dangling_vec_begin() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_vec_data() -> int4* {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_vec_subscript() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_vec_front() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_vec_at() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.at(0u).value();
}

void
use_after_scope_vec() {
   int4* p_escaped = nullptr;
   {
      cat::vec<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}

auto
dangling_vec_inplace_begin() -> auto {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_vec_inplace_data() -> int4* {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_vec_inplace_subscript() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_vec_inplace_front() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_vec_inplace_at() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.at(0u).value();
}

void
use_after_scope_vec_inplace() {
   int4* p_escaped = nullptr;
   {
      cat::vec_inplace<int4, 4u> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}

auto
dangling_raii_vec_begin() -> auto {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_vec_data() -> int4* {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_vec_subscript() -> int4& {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_raii_vec_front() -> int4& {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_raii_vec_at() -> int4& {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.at(0u).value();
}

void
use_after_scope_raii_vec() {
   int4* p_escaped = nullptr;
   {
      cat::raii::vec<int4, cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}
