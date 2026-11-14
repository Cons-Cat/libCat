// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations in `<cat/maybe>`. Any `expected-error`
// checks must come last, because Clang stops its `-Wlifetime-safety` pass after
// the first error in a translation unit. Run with `just test safety`.

#include <cat/maybe>

using namespace cat::literals;
using namespace cat::arithmetic;

inline int4 g_sentinel_slot = 0;

using custom_ptr_maybe = cat::maybe<cat::sentinel<int4*, &g_sentinel_slot>>;

auto
borrow_maybe_ref(int4& value [[clang::lifetimebound]]) -> cat::maybe<int4&> {
   return value;
}

auto
borrow_plain_ptr_maybe(int4* p_value [[clang::lifetimebound]])
   -> cat::maybe<int4*> {
   return p_value;
}

auto
borrow_maybe_ptr(int4* p_value [[clang::lifetimebound]])
   -> cat::maybe_ptr<int4> {
   return p_value;
}

auto
borrow_custom_ptr_maybe(int4* p_value [[clang::lifetimebound]])
   -> custom_ptr_maybe {
   return p_value;
}

inline constexpr int4 g_constant = 3;

// Deleted temporary-binding constructors are verified via `static_assert` so
// this TU stays error-free and `-Wlifetime-safety` analyzes every function.
static_assert(!cat::is_constructible<cat::maybe<int4 const&>, int4>);

static_assert(
   !cat::is_constructible<cat::maybe<int4 const&>, cat::maybe<int4>>
);

// Positive cases: valid temporary unwrapping and lifetime propagation.
// Every statement below must compile with ZERO warnings.

void
valid_maybe_temporary_unwrapping() {
   int4 target = 42;

   // `maybe<T&>` temporary unwrapping.
   int4& ref_value = borrow_maybe_ref(target).value();
   int4& ref_verified = borrow_maybe_ref(target).verify();
   int4& ref_asserted = borrow_maybe_ref(target).assert();
   int4* p_ref = borrow_maybe_ref(target).get_ptr();
   int4* p_ref_data = borrow_maybe_ref(target).data();
   int4* p_ref_begin = borrow_maybe_ref(target).begin();
   int4* p_ref_end = borrow_maybe_ref(target).end();

   // Pointer `maybe` temporary unwrapping across all three storage regimes,
   // including implicit `T*` -> `T const*` conversion.
   cat::maybe<int4 const*> const_plain_maybe = &target;
   cat::maybe_ptr<int4 const> const_compact_maybe = &target;
   int4 const* p_const_plain = const_plain_maybe.verify();
   int4 const* p_const_compact = const_compact_maybe.verify();

   int4* p_plain_value = borrow_plain_ptr_maybe(&target).value();
   int4* p_plain_verified = borrow_plain_ptr_maybe(&target).verify();
   int4* p_plain_asserted = borrow_plain_ptr_maybe(&target).assert();
   int4* p_plain = borrow_plain_ptr_maybe(&target).get_ptr();
   int4* p_plain_or = borrow_plain_ptr_maybe(&target).value_or(&target);

   int4* p_compact_value = borrow_maybe_ptr(&target).value();
   int4* p_compact_verified = borrow_maybe_ptr(&target).verify();
   int4* p_compact_asserted = borrow_maybe_ptr(&target).assert();
   int4* p_compact = borrow_maybe_ptr(&target).get_ptr();

   int4* p_custom_value = borrow_custom_ptr_maybe(&target).value();
   int4* p_custom_verified = borrow_custom_ptr_maybe(&target).verify();
   int4* p_custom_asserted = borrow_custom_ptr_maybe(&target).assert();
   int4* p_custom = borrow_custom_ptr_maybe(&target).get_ptr();
}

auto
valid_outliving_via_local_maybe_ref(int4& caller_value [[clang::lifetimebound]])
   -> int4& {
   cat::maybe<int4&> local_ref = caller_value;
   return local_ref.verify();
}

auto
valid_outliving_via_local_ptr_var(int4& caller_value [[clang::lifetimebound]])
   -> cat::maybe<int4*> {
   // `maybe(pointer)` takes `pointer` by value, so lifetime ties to
   // `caller_value` rather than to the local pointer variable `p_local`.
   int4* p_local = &caller_value;
   return cat::maybe<int4*>{p_local};
}

// Negative cases: `maybe<T&>` (`[[gsl::Pointer(T)]]`).

auto
dangling_maybe_ref_value() -> int4& {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe<int4&> maybe = local;
   return maybe.value();
}

auto
dangling_maybe_ref_direct() -> cat::maybe<int4&> {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe<int4&> maybe = local;
   return maybe;
}

auto
dangling_maybe_ref_verify() -> int4& {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::maybe<int4&>(local).verify();
}

auto
dangling_maybe_ref_assert() -> int4& {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe<int4&> maybe = local;
   return maybe.assert();
}

auto
dangling_maybe_ref_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe<int4&> maybe = local;
   return maybe.get_ptr();
}

auto
dangling_maybe_ref_data() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::maybe<int4&>(local).data();
}

auto
dangling_maybe_ref_begin() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::maybe<int4&>(local).begin();
}

void
use_after_scope_maybe_ref() {
   int4* p_escaped = nullptr;
   {
      int4 local = 1;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      cat::maybe<int4&> maybe = local;
      p_escaped = maybe.get_ptr();
   }
   *p_escaped = 2;
}

// Negative cases: pointer `maybe<T>` (`[[gsl::Pointer]]`).

auto
dangling_compact_maybe_ptr_value() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::maybe_ptr<int4>(&local).value();
}

auto
dangling_custom_ptr_maybe_value() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   custom_ptr_maybe maybe = &local;
   return maybe.value();
}

auto
dangling_custom_ptr_maybe_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   custom_ptr_maybe maybe = &local;
   return maybe.get_ptr();
}

auto
dangling_plain_maybe_ptr_via_local_ptr() -> cat::maybe<int4*> {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   int4* p_local = &local;
   return cat::maybe<int4*>{p_local};
}

auto
dangling_plain_maybe_ptr_value() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::maybe<int4*>(&local).value();
}

auto
dangling_plain_maybe_ptr_verify() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe<int4*> maybe = &local;
   return maybe.verify();
}

auto
dangling_plain_maybe_ptr_assert() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::maybe<int4*>(&local).assert();
}

auto
dangling_plain_maybe_ptr_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe<int4*> maybe = &local;
   return maybe.get_ptr();
}

auto
dangling_compact_maybe_ptr_direct() -> cat::maybe_ptr<int4> {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return &local;
}

auto
dangling_compact_maybe_ptr_verify() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return cat::maybe_ptr<int4>(&local).verify();
}

auto
dangling_compact_maybe_ptr_assert() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe_ptr<int4> maybe = &local;
   return maybe.assert();
}

auto
dangling_compact_maybe_ptr_get_ptr() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   cat::maybe_ptr<int4> maybe = &local;
   return maybe.get_ptr();
}

auto
dangling_custom_ptr_maybe_direct() -> custom_ptr_maybe {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return &local;
}

auto
dangling_custom_ptr_maybe_verify() -> int4* {
   int4 local = 1;
   // expected-warning@+1 2 {{local variable 'local'}}
   return custom_ptr_maybe(&local).verify();
}

auto
dangling_custom_ptr_maybe_assert() -> int4* {
   int4 local = 1;
   // expected-warning@+1 {{local variable 'local' is returned}}
   custom_ptr_maybe maybe = &local;
   return maybe.assert();
}

void
use_after_scope_maybe_ptr() {
   int4* p_escaped = nullptr;
   {
      int4 local = 1;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      cat::maybe_ptr<int4> maybe = &local;
      p_escaped = maybe.get_ptr();
   }
   *p_escaped = 2;
}

// Negative cases: value-owning `maybe`.

void
dangling_from_owning_maybe_temporaries() {
   // expected-warning@+1 {{local reference 'value_ref'}}
   int4 const& value_ref = cat::maybe<int4>{1}.value();
   // expected-warning@+1 {{local reference 'verified_ref'}}
   int4 const& verified_ref = cat::maybe<int4>{1}.verify();
   // expected-warning@+1 {{local variable 'p_data'}}
   int4* p_data = cat::maybe<int4>{1}.data();
}

auto
dangling_owning_maybe_local() -> int4& {
   cat::maybe<int4> local_maybe = 1;
   // expected-warning@+1 2 {{local variable 'local_maybe'}}
   return local_maybe.verify();
}

// Constant evaluation of the by-value `this ... self [[lifetimebound]]`
// accessors, which a Clang crash was once suspected in.

static_assert(cat::maybe<int4 const&>(g_constant).value() == 3);

static_assert(cat::maybe<int4 const&>(g_constant).get_ptr() == &g_constant);

static_assert(*cat::maybe<int4 const&>(g_constant).begin() == 3);

static_assert(cat::maybe<int4 const*>(&g_constant).value() == &g_constant);

static_assert(cat::maybe<int4 const*>(&g_constant).get_ptr() == &g_constant);

static_assert(cat::maybe_ptr<int4 const>(&g_constant).value() == &g_constant);

static_assert(cat::maybe_ptr<int4 const>(&g_constant).get_ptr() == &g_constant);

// A `maybe` of a reference never binds to an r-value, because it could not
// extend the r-value's lifetime.
void
maybe_reference_rejects_rvalues() {
   int4 value = 1;
   int raw = 1;

   // expected-error@+1 {{}}
   cat::maybe<int4&> from_prvalue(int4{1});
   // expected-error@+1 {{}}
   cat::maybe<int4&> from_xvalue(cat::move(value));
   // An `int4&` cannot bind to an `int`, although an `int4` is assignable from
   // one.
   // expected-error@+1 {{}}
   cat::maybe<int4&> from_other_type(raw);
   // expected-error@+1 {{}}
   cat::maybe<int4 const&> from_const_prvalue(int4{1});
   // Binding `int4 const&` to an `int` would create a temporary `int4`.
   // expected-error@+1 {{}}
   cat::maybe<int4 const&> from_conversion(raw);

   cat::maybe<int4&> reference = value;
   cat::maybe<int4 const&> const_reference = value;
   // expected-error@+1 {{}}
   reference = raw;
   // expected-error@+1 {{}}
   const_reference = int4{2};
   // expected-error@+1 {{}}
   const_reference = raw;

   // Assigning to a temporary would only rebind the temporary.
   // expected-error@+1 {{immediately dangles}}
   (cat::maybe<int4&>(value)) = value;
}
