// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations and the constant bounds checks in
// `<cat/tuple>`. Run with `just test safety`.

#include <cat/tuple>

using namespace cat::literals;
using namespace cat::arithmetic;

// `tuple` element accessors. A value element is borrowed from the `tuple`,
// and a reference element's referent can outlive it.

auto
valid_tuple_ref_first(int4& caller_value [[clang::lifetimebound]]) -> int4& {
   return cat::tuple<int4&>{caller_value}.first();
}

auto
valid_tuple_ref_get(int4& caller_value [[clang::lifetimebound]]) -> int4& {
   return cat::tuple<int4&>{caller_value}.get<0u>();
}

auto
valid_tuple_ref_free_get(int4& caller_value [[clang::lifetimebound]]) -> int4& {
   return cat::get<0u>(cat::tuple<int4&, int4>{caller_value, 2_i4});
}

auto
dangling_tuple_value_first() -> int4& {
   cat::tuple<int4, int4> local_tuple{1_i4, 2_i4};
   // expected-warning@+1 2 {{local variable 'local_tuple'}}
   return local_tuple.first();
}

auto
dangling_tuple_value_get() -> int4& {
   cat::tuple<int4, int4> local_tuple{1_i4, 2_i4};
   // expected-warning@+1 2 {{local variable 'local_tuple'}}
   return local_tuple.get<1u>();
}

auto
dangling_tuple_value_free_get() -> int4& {
   cat::tuple<int4, int4> local_tuple{1_i4, 2_i4};
   // expected-warning@+1 2 {{local variable 'local_tuple'}}
   return cat::get<0u>(local_tuple);
}

auto
dangling_tuple_mixed_value_second(int4& caller_value) -> int4& {
   cat::tuple<int4&, int4> local_tuple{caller_value, 2_i4};
   // expected-warning@+1 2 {{local variable 'local_tuple'}}
   return local_tuple.second();
}

// Constant bounds checks. These must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit.

void
tuple_constant_bounds(idx runtime_index) {
   cat::tuple<int4, int4> pair{1_i4, 2_i4};
   auto _ = pair[1u] + pair.get<1u>();

   // expected-error@+1 {{no viable overloaded operator[]}}
   auto _ = pair[2u];
   // expected-error@+1 {{no viable overloaded operator[]}}
   auto _ = pair[runtime_index];
   // expected-error@+1 {{no matching member function for call to 'get'}}
   auto _ = pair.get<2u>();
}
