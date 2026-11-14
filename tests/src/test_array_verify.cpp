// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations and the constant bounds checks in
// `<cat/array>`. Run with `just test safety`.

#include <cat/array>

using namespace cat::literals;
using namespace cat::arithmetic;

// Negative cases: `maybe<T&>` (`[[gsl::Pointer(T)]]`).

auto
dangling_container_at_value() -> int4& {
   cat::array<int4, 2u> list = {1, 2};
   // expected-warning@+1 2 {{local variable 'list'}}
   return list.at(0u).value();
}

void
use_after_scope_container_at_value() {
   int4* p_escaped = nullptr;
   {
      cat::array<int4, 2u> list = {1, 2};
      // expected-warning@+1 {{local variable 'list' does not live long}}
      p_escaped = &list.at(0u).value();
   }
   *p_escaped = 2;
}

// Constant bounds checks. These must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit.

void
array_constant_bounds() {
   cat::array<int4, 4u> values{};
   cat::array<int4, 4u> const constant{};
   auto _ = values[3u] + constant[3u] + cat::array<int4, 4u>{}[3u];

   // expected-error@cat/container:* 4 {{Index is out of bounds!}}
   values[4u];
   constant[4u];
   cat::move(values)[4u];
   cat::move(constant)[4u];
}
