// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations in `<cat/scaredy>`. Any
// `expected-error` checks must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit. Run
// with `just test safety`.

#include <cat/scaredy>

using namespace cat::literals;
using namespace cat::arithmetic;

struct err_code {
   int4 code;
};

enum class compact_ptr_err : unsigned char {
   null_ptr = 0u,
};

using compact_ptr_scaredy = cat::scaredy<
   cat::compact_scaredy<
      int4*,
      [](int4* p_value) -> bool {
         return p_value != nullptr;
      }>,
   compact_ptr_err>;

auto
borrow_ptr_scaredy(int4* p_value [[clang::lifetimebound]])
   -> cat::scaredy<int4*, err_code> {
   return p_value;
}

auto
borrow_compact_ptr_scaredy(int4* p_value [[clang::lifetimebound]])
   -> compact_ptr_scaredy {
   return p_value;
}

inline constexpr int4 g_constant = 3;

// Positive cases: valid temporary unwrapping and lifetime propagation.
// Every statement below must compile with ZERO warnings.

void
valid_scaredy_temporary_unwrapping() {
   int4 target = 42;

   // Pointer `scaredy` temporary unwrapping (plain and compact).
   cat::scaredy<int4 const*, err_code> const_scaredy = &target;
   int4 const* p_const = const_scaredy.verify();

   int4* p_plain_value = borrow_ptr_scaredy(&target).value();
   int4* p_plain_verified = borrow_ptr_scaredy(&target).verify();
   int4* p_plain_asserted = borrow_ptr_scaredy(&target).assert();
   int4* p_plain = borrow_ptr_scaredy(&target).get_ptr();
   int4* p_plain_or = borrow_ptr_scaredy(&target).value_or(&target);

   int4* p_compact_value = borrow_compact_ptr_scaredy(&target).value();
   int4* p_compact_verified = borrow_compact_ptr_scaredy(&target).verify();
   int4* p_compact_asserted = borrow_compact_ptr_scaredy(&target).assert();
   int4* p_compact = borrow_compact_ptr_scaredy(&target).get_ptr();
}

auto
valid_outliving_via_local_scaredy_ptr(
   int4& caller_value [[clang::lifetimebound]]
) -> int4* {
   int4* p_local = &caller_value;
   cat::scaredy<int4*, err_code> local_result = p_local;
   return local_result.verify();
}

// Negative cases: pointer `scaredy<T, Errors...>` (`[[gsl::Pointer]]`).

auto
dangling_compact_scaredy_ptr_value() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return compact_ptr_scaredy(&local).value();
}

auto
dangling_compact_scaredy_ptr_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   compact_ptr_scaredy scaredy = &local;
   return scaredy.get_ptr();
}

auto
dangling_scaredy_ptr_direct() -> cat::scaredy<int4*, err_code> {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   int4* p_local = &local;
   return p_local;
}

auto
dangling_scaredy_ptr_value() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::scaredy<int4*, err_code>(&local).value();
}

auto
dangling_scaredy_ptr_verify() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::scaredy<int4*, err_code> scaredy = &local;
   return scaredy.verify();
}

auto
dangling_scaredy_ptr_assert() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::scaredy<int4*, err_code>(&local).assert();
}

auto
dangling_scaredy_ptr_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::scaredy<int4*, err_code> scaredy = &local;
   return scaredy.get_ptr();
}

auto
dangling_compact_scaredy_ptr_direct() -> compact_ptr_scaredy {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return &local;
}

auto
dangling_compact_scaredy_ptr_verify() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return compact_ptr_scaredy(&local).verify();
}

auto
dangling_compact_scaredy_ptr_assert() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   compact_ptr_scaredy scaredy = &local;
   return scaredy.assert();
}

void
use_after_scope_scaredy_ptr() {
   int4* p_escaped = nullptr;
   {
      int4 local = 1;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      cat::scaredy<int4*, err_code> scaredy = &local;
      p_escaped = scaredy.get_ptr();
   }
   *p_escaped = 2;
}

// Negative cases: value-owning `scaredy`.

void
dangling_from_owning_scaredy_temporaries() {
   // expected-warning@+1 {{local reference 'value_ref'}}
   int4 const& value_ref = cat::scaredy<int4, err_code>{1}.value();
   // expected-warning@+1 {{local reference 'verified_ref'}}
   int4 const& verified_ref = cat::scaredy<int4, err_code>{1}.verify();
}

auto
dangling_owning_scaredy_local() -> int4& {
   cat::scaredy<int4, err_code> local_scaredy = 1;
   // expected-warning@+1 2 {{local variable 'local_scaredy'}}
   return local_scaredy.verify();
}

// Constant evaluation of the by-value `this ... self [[lifetimebound]]`
// accessors, which a Clang crash was once suspected in.

static_assert(
   cat::scaredy<int4 const*, err_code>(&g_constant).value() == &g_constant
);
