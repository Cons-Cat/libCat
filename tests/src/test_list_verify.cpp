// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/list>`, `<cat/forward_list>`,
// and their `raii` versions. Any `expected-error` checks must come last,
// because Clang stops its `-Wlifetime-safety` pass after the first error in a
// translation unit. Run with `just test safety`.

#include <cat/allocator_page>
#include <cat/forward_list>
#include <cat/list>
#include <cat/raii_forward_list>
#include <cat/raii_list>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Positive cases.

auto
valid_list_element_copy() -> int4 {
   cat::list<int4> local;
   return local.front();
}

// Negative cases: element references and iterators outlive their list.

auto
dangling_list_begin() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_list_front() -> int4& {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_list() {
   int4* p_escaped = nullptr;
   {
      cat::list<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}

auto
dangling_forward_list_begin() -> auto {
   cat::forward_list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_forward_list_front() -> int4& {
   cat::forward_list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_forward_list() {
   int4* p_escaped = nullptr;
   {
      cat::forward_list<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}

auto
dangling_raii_list_begin() -> auto {
   cat::raii::list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_list_front() -> int4& {
   cat::raii::list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_raii_list() {
   int4* p_escaped = nullptr;
   {
      cat::raii::list<int4, cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}

auto
dangling_raii_forward_list_begin() -> auto {
   cat::raii::forward_list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_forward_list_front() -> int4& {
   cat::raii::forward_list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_raii_forward_list() {
   int4* p_escaped = nullptr;
   {
      cat::raii::forward_list<int4, cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}
