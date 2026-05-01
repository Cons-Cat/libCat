#include <cat/array>
#include <cat/span>

#include "../unit_tests.hpp"

namespace {

consteval auto
test_constexpr_pointer_in_range() -> bool {
   cat::array<int4, 4> values{11, 22, 33, 44};
   cat::span<int4 const> const range = values;

   if (!cat::is_pointer_in_range(values.data(), range)) {
      return false;
   }
   if (!cat::is_pointer_in_range(&values[3], range)) {
      return false;
   }
   if (cat::is_pointer_in_range(values.data() + values.size(), range)) {
      return false;
   }

   int4 const separate_value = 123;
   return !cat::is_pointer_in_range(&separate_value, range);
}

static_assert(test_constexpr_pointer_in_range());

}  // namespace

$test(pointer_in_range) {
   cat::array<int4, 8> values{0, 1, 2, 3, 4, 5, 6, 7};
   cat::span<int4 const> const range = values;

   for (idx i = 0u; i < range.size(); ++i) {
      cat::verify(
         cat::is_pointer_in_range(__builtin_addressof(values[i]), range));
   }

   cat::verify(!cat::is_pointer_in_range(values.data() + values.size(), range));

   int4 outside = 99;
   cat::verify(!cat::is_pointer_in_range(__builtin_addressof(outside), range));

   cat::span<int4 const> const empty{};
   cat::verify(!cat::is_pointer_in_range(values.data(), empty));
}
