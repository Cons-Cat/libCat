// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations and the constant bounds checks in
// `<cat/span>`. Run with `just test safety`.

#include <cat/array>
#include <cat/span>

using namespace cat::literals;
using namespace cat::arithmetic;

auto
borrow_fixed_span(cat::array<int4, 2u>& array [[clang::lifetimebound]])
   -> cat::span<int4, 2u> {
   return array;
}

static_assert(
   !cat::is_constructible<cat::span<int4 const>, cat::array<int4, 2u>>
);

// Positive cases: valid temporary unwrapping and lifetime propagation.
// Every statement below must compile with ZERO warnings.

void
valid_span_temporary_unwrapping() {
   int4 target = 42;
   cat::array<int4, 2u> array{1_i4, 2_i4};
   cat::span<int4> dynamic_span = array;

   // `span` temporary unwrapping.
   int4& subscripted = borrow_fixed_span(array)[0u];
   int4& got = cat::get<0u>(borrow_fixed_span(array));
   int4& verified = dynamic_span.at(0u).verify();
}

// Negative cases: value-owning `span`.

auto
dangling_span_direct() -> cat::span<int4> {
   cat::array<int4, 2u> local_array{1_i4, 2_i4};
   // expected-warning@+1 {{local variable 'local_array' is returned}}
   cat::span<int4> span = local_array;
   return span;
}

auto
valid_span_temporary_front_back_at(
   cat::span<int4> caller_span [[clang::lifetimebound]]
) -> int4& {
   int4& front = cat::span<int4>(caller_span).front();
   [[maybe_unused]]
   int4& back = cat::span<int4>(caller_span).back();
   auto _ = cat::span<int4>(caller_span).at(0u).value();
   return front;
}

auto
dangling_span_local_front() -> int4& {
   cat::array<int4, 2u> local_array{1_i4, 2_i4};
   // expected-warning@+1 {{local variable 'local_array' is returned}}
   cat::span<int4> span = local_array;
   return span.front();
}

auto
dangling_span_local_back() -> int4& {
   cat::array<int4, 2u> local_array{1_i4, 2_i4};
   // expected-warning@+1 {{local variable 'local_array' is returned}}
   cat::span<int4> span = local_array;
   return span.back();
}

auto
dangling_span_local_at() -> int4& {
   cat::array<int4, 2u> local_array{1_i4, 2_i4};
   // expected-warning@+1 {{local variable 'local_array' is returned}}
   cat::span<int4> span = local_array;
   return span.at(0u).value();
}

auto
dangling_span_local_get() -> int4& {
   cat::array<int4, 2u> local_array{1_i4, 2_i4};
   // expected-warning@+1 {{local variable 'local_array' is returned}}
   cat::span<int4, 2u> span = local_array;
   return cat::get<0u>(span);
}

// Constant evaluation of the by-value `this ... self [[lifetimebound]]`
// accessors, which a Clang crash was once suspected in.

static_assert([] {
   cat::array<int4, 2u> values{1_i4, 2_i4};
   cat::span<int4> view = values;
   view.front() = 5_i4;
   return view[0u] + view.back() + view.at(1u).value();
}() == 9);

// Constant bounds checks. These must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit.

void
span_constant_bounds() {
   cat::array<int4, 4u> values{};
   cat::span<int4, 4_idx> fixed = values;
   cat::span<int4, 4_idx> const constant = values;
   cat::span<int4> dynamic = values;
   auto _ = fixed[3u] + constant[3u] + cat::move(fixed)[3u] + dynamic[9u];

   // One guard serves every value category, so its `static_assert` fires once.
   // expected-error@cat/span:* {{Index is out of bounds!}}
   fixed[4u];
   constant[4u];
   cat::move(fixed)[4u];
}
