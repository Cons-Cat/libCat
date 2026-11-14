// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations in `<cat/variant>`. Any
// `expected-error` checks must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in first translation unit. Run
// with `just test safety`.

#include <cat/maybe>
#include <cat/variant>

using namespace cat::literals;
using namespace cat::arithmetic;

struct derived_ref_variant : cat::variant<int4&, double&> {
   using cat::variant<int4&, double&>::variant;
};

auto
borrow_ref_variant(int4& value [[clang::lifetimebound]])
   -> cat::variant<int4&, double&> {
   return value;
}

auto
borrow_derived_ref_variant(int4& value [[clang::lifetimebound]])
   -> derived_ref_variant {
   return value;
}

static_assert(
   !cat::is_constructible<cat::variant<int4 const&, double const&>, int4>
);

// Positive cases: valid temporary unwrapping and lifetime propagation.
// Every statement below must compile with ZERO warnings.

void
valid_variant_temporary_unwrapping() {
   int4 target = 42;

   // Reference `variant` temporary unwrapping (member, free, and P2162
   // derived).
   int4& member_get_ref = borrow_ref_variant(target).get<int4&>();
   int4& member_get_bare = borrow_ref_variant(target).get<int4>();
   int4& member_get_index = borrow_ref_variant(target).get<0u>();
   int4* p_member = borrow_ref_variant(target).get_ptr<int4&>();
   cat::maybe<int4&> member_get_if = borrow_ref_variant(target).get_if<int4>();

   int4& free_get_ref = cat::get<int4&>(borrow_ref_variant(target));
   int4& free_get_index = cat::get<0u>(borrow_ref_variant(target));
   int4* p_free = cat::get_ptr<int4&>(borrow_ref_variant(target));
   int4* p_free_index = cat::get_ptr<0u>(borrow_ref_variant(target));
   cat::maybe<int4&> free_get_if =
      cat::get_if<int4>(borrow_ref_variant(target));
   cat::maybe<int4&> free_get_if_index =
      cat::get_if<0u>(borrow_ref_variant(target));

   int4& derived_get = cat::get<int4&>(borrow_derived_ref_variant(target));
   int4* p_derived = cat::get_ptr<0u>(borrow_derived_ref_variant(target));
   cat::maybe<int4&> derived_get_if =
      cat::get_if<int4>(borrow_derived_ref_variant(target));
}

auto
valid_outliving_via_local_ref_variant(
   int4& caller_value [[clang::lifetimebound]]
) -> int4& {
   cat::variant<int4&, double&> local_variant(caller_value);
   return cat::get<int4&>(local_variant);
}

auto
valid_ref_variant_rebind(
   int4& first [[clang::lifetimebound]], int4& second [[clang::lifetimebound]]
) -> int4& {
   cat::variant<int4&, double&> variant(first);
   variant = second;
   return variant.get<int4&>();
}

auto
dangling_ref_variant_rebind(int4& outer) -> int4& {
   cat::variant<int4&, double&> variant(outer);
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   variant = local;
   return variant.get<int4&>();
}

void
use_after_scope_ref_variant_rebind(int4& outer) {
   cat::variant<int4&, double&> variant(outer);
   {
      int4 local = 1;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      variant = local;
   }
   variant.get<int4&>() = 2;
}

// Negative cases: reference `variant<Alternatives...>` (`[[gsl::Pointer]]`).

auto
dangling_ref_variant_direct() -> cat::variant<int4&, double&> {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::variant<int4&, double&> variant(local);
   return variant;
}

auto
dangling_ref_variant_member_get() -> int4& {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::variant<int4&, double&> variant(local);
   return variant.get<int4&>();
}

auto
dangling_ref_variant_member_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::variant<int4&, double&> variant(local);
   return variant.get_ptr<0u>();
}

auto
dangling_ref_variant_member_get_if() -> cat::maybe<int4&> {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::variant<int4&, double&> variant(local);
   return variant.get_if<int4>();
}

auto
dangling_ref_variant_free_get_lvalue() -> int4& {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::variant<int4&, double&> variant(local);
   return cat::get<int4&>(variant);
}

auto
dangling_ref_variant_free_get_rvalue() -> int4& {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::get<0u>(cat::variant<int4&, double&>(local));
}

auto
dangling_ref_variant_free_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::variant<int4&, double&> variant(local);
   return cat::get_ptr<int4&>(variant);
}

auto
dangling_ref_variant_free_get_if() -> cat::maybe<int4&> {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::variant<int4&, double&> variant(local);
   return cat::get_if<int4>(variant);
}

auto
dangling_derived_ref_variant_free_get() -> int4& {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::get<int4&>(derived_ref_variant(local));
}

void
use_after_scope_ref_variant_free_get_ptr() {
   int4* p_escaped = nullptr;
   {
      int4 local = 1;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      cat::variant<int4&, double&> variant(local);
      p_escaped = cat::get_ptr<int4&>(variant);
   }
   *p_escaped = 2;
}

// Negative cases: value-owning `variant`.

void
dangling_from_owning_variant_temporaries() {
   // expected-warning@+1 {{local reference 'member_ref'}}
   int4 const& member_ref = cat::variant<int4, double>{1}.get<int4>();
   // expected-warning@+1 {{local reference 'free_ref'}}
   int4 const& free_ref = cat::get<int4>(cat::variant<int4, double>{1});
   // expected-warning@+1 {{local variable 'p_alternative'}}
   int4* p_alternative = cat::get_ptr<int4>(cat::variant<int4, double>{1});
}

auto
dangling_owning_variant_local() -> int4& {
   cat::variant<int4, double> local_variant = 1;
   // expected-warning@+1 2 {{local variable 'local_variant'}}
   return cat::get<int4>(local_variant);
}
