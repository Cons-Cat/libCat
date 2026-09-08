#include <cat/format>
#include <cat/linear_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"
#include "quantity_format_goldens.hpp"

namespace cat::units {

#define CAT_DECLARE_QUANTITY_FORMAT_GOLDEN(name, integer, floating) \
   inline constexpr bool name##_has_quantity_format_golden = true;
CAT_QUANTITY_FORMAT_TESTS(CAT_DECLARE_QUANTITY_FORMAT_GOLDEN)
#undef CAT_DECLARE_QUANTITY_FORMAT_GOLDEN

}  // namespace cat::units

#undef CAT_QUANTITY_ALIAS_HOOK
#define CAT_QUANTITY_ALIAS_HOOK(name, quantity)                    \
   static_assert(                                                  \
      name##_has_quantity_format_golden,                           \
      "quantity alias requires an explicit formatting golden case" \
   )

#include <cat/quantity>

namespace {

template <typename Integer, typename Floating>
void
verify_quantity_format(
   cat::str_view integer_expected, cat::str_view floating_expected
) {
   cat::span page = pager.alloc_multi<cat::byte>(1_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);
   auto const integer_actual = cat::fmt(allocator, "{}", Integer(1)).verify();
   auto const floating_actual = cat::fmt(allocator, "{}", Floating(1)).verify();
   cat::verify(integer_actual == integer_expected);
   cat::verify(floating_actual == floating_expected);
}

}  // namespace

$test(quantity_format_catalog) {
#define CAT_VERIFY_QUANTITY_FORMAT_GOLDEN(name, integer, floating)             \
   verify_quantity_format<cat::units::name##_int4, cat::units::name##_float8>( \
      integer, floating                                                        \
   );
   CAT_QUANTITY_FORMAT_TESTS(CAT_VERIFY_QUANTITY_FORMAT_GOLDEN)
#undef CAT_VERIFY_QUANTITY_FORMAT_GOLDEN
#undef CAT_QUANTITY_FORMAT_TESTS
}
