// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/string>`. Any
// `expected-error` checks must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit. Run
// with `just test safety`.

#include <cat/allocator_page>
#include <cat/string>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Positive cases.

auto
valid_str_view_of_literal() -> cat::str_view {
   return "literal";
}

auto
valid_str_vec_borrow(cat::str_vec& caller_string [[clang::lifetimebound]])
   -> cat::str_view {
   return caller_string;
}

// Negative cases: characters, views, and iterators outlive their string.

auto
dangling_str_inplace_view() -> cat::str_view {
   cat::str_inplace<8u> local = "abc";
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local;
}

auto
dangling_str_vec_begin() -> auto {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_str_vec_data() -> auto {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_str_vec_subscript() -> auto& {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_str_vec_front() -> auto& {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_str_vec() {
   char const* p_escaped = nullptr;
   {
      cat::str_vec local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   auto _ = *p_escaped;
}

auto
dangling_str_inplace_begin() -> auto {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_str_inplace_data() -> auto {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_str_inplace_subscript() -> auto& {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_str_inplace_front() -> auto& {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_str_inplace() {
   char const* p_escaped = nullptr;
   {
      cat::str_inplace<8u> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   auto _ = *p_escaped;
}

auto
dangling_raii_str_vec_begin() -> auto {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_str_vec_data() -> auto {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_str_vec_subscript() -> auto& {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_raii_str_vec_front() -> auto& {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_raii_str_vec() {
   char const* p_escaped = nullptr;
   {
      cat::raii::str_vec<cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   auto _ = *p_escaped;
}
