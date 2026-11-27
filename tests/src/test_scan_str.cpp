#include <cat/allocator_linear>
#include <cat/scan_str>
#include <cat/string>
#include <cat/type_list>

#include "../unit_tests.hpp"
#include "integer_bounds.hpp"

namespace {

// Non-owning strings have a deleted `scanner`.
template <typename T>
concept has_scanner =
   requires(cat::allocator_ref<cat::allocator_dyn> allocator) {
      cat::scanner<T>{}.scan(allocator, cat::str_view());
   };

static_assert(has_scanner<int4>);
static_assert(has_scanner<cat::idx>);
static_assert(!has_scanner<cat::ualign>);
static_assert(has_scanner<cat::str_inplace<8u>>);
static_assert(has_scanner<cat::str_vec>);
static_assert(has_scanner<cat::raii::str_vec<>>);
static_assert(!has_scanner<cat::str_view>);
static_assert(!has_scanner<cat::zstr_view>);
static_assert(!has_scanner<cat::str_span>);
static_assert(!has_scanner<cat::zstr_span>);

static_assert(cat::is_scannable_without_allocator<int4>);
static_assert(cat::is_scannable_without_allocator<cat::str_inplace<8u>>);
static_assert(!cat::is_scannable_without_allocator<cat::str_vec>);
static_assert(!cat::is_scannable_without_allocator<cat::raii::str_vec<>>);
static_assert(!cat::is_scannable_without_allocator<cat::str_view>);
static_assert(cat::scan_str<int4>("42").value() == 42);

template <typename T>
void
verify_scan_integer(
   cat::allocator_dyn allocator, integer_bounds bounds = bounds_of<T>()
) {
   auto scan = [&](cat::str_view input) -> T {
      return cat::scan_str<T>(allocator, input).verify();
   };

   cat::verify(scan("0") == 0);
   cat::verify(scan("42") == 42);
   if constexpr (cat::is_signed<T>) {
      cat::verify(scan("-42") == -42);
   }
   cat::verify(scan(bounds.min) == cat::limits<T>::min());
   cat::verify(scan(bounds.max) == cat::limits<T>::max());
   cat::verify(cat::scan_str<T>(allocator, bounds.below_min).is_empty());
   cat::verify(cat::scan_str<T>(allocator, bounds.above_max).is_empty());
}
}  // namespace

$test(scan_str_integers) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   cat::allocator_linear linear = cat::make_allocator_linear(page);
   cat::allocator_dyn allocator = linear;

   [&]<typename... Ts>(cat::type_list<Ts...>) {
      (verify_scan_integer<Ts>(allocator), ...);
   }(parseable_integers{});

   verify_scan_integer<cat::idx>(allocator, idx_bounds);

   // Malformed or partially consumed.
   cat::verify(cat::scan_str<int4>(allocator, "").is_empty());
   cat::verify(cat::scan_str<int4>(allocator, "-").is_empty());
   cat::verify(cat::scan_str<int4>(allocator, "+1").is_empty());
   cat::verify(cat::scan_str<int4>(allocator, " 1").is_empty());
   cat::verify(cat::scan_str<int4>(allocator, "12a").is_empty());

   g_tests_alloc.free(page);
}

$test(scan_str_strings) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   cat::allocator_linear linear = cat::make_allocator_linear(page);
   cat::allocator_dyn allocator = linear;
   cat::str_view const input = "hello, world";

   cat::str_inplace<16u> inplace =
      cat::scan_str<cat::str_inplace<16u>>(allocator, input).verify();
   cat::verify(inplace == input);
   cat::verify(
      cat::scan_str<cat::str_inplace<4u>>(allocator, input).is_empty()
   );

   cat::str_vec manual = cat::scan_str<cat::str_vec>(allocator, input).verify();
   cat::verify(manual == input);
   manual.free(allocator);

   cat::zstr_vec terminated =
      cat::scan_str<cat::zstr_vec>(allocator, input).verify();
   cat::verify(terminated == input);
   terminated.free(allocator);

   cat::raii::str_vec<> owning =
      cat::scan_str<cat::raii::str_vec<>>(allocator, input).verify();
   cat::verify(owning == input);

   cat::raii::str_vec<cat::allocator_linear> typed =
      cat::scan_str<
         cat::raii::str_vec<cat::allocator_linear>, cat::allocator_linear>(
         linear, input
      )
         .verify();
   cat::verify(typed == input);

   g_tests_alloc.free(page);
}

static_assert(cat::scan_str<bool>("true").verify());
static_assert(!cat::scan_str<bool>("false").verify());
static_assert(cat::scan_str<bool>("1").is_empty());
static_assert(cat::scan_str<bool>("True").is_empty());
static_assert(cat::scan_str<bool>("").is_empty());
static_assert(cat::scan_str<bool2>("true").verify());
static_assert(!cat::scan_str<bool2>("false").verify());
static_assert(cat::scan_str<bool2>("1").is_empty());
static_assert(cat::scan_str<bool4>("true").verify());
static_assert(!cat::scan_str<bool4>("false").verify());
static_assert(cat::scan_str<bool4>("1").is_empty());

$test(scan_str_quoted_strings) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   cat::allocator_linear linear = cat::make_allocator_linear(page);
   cat::allocator_dyn allocator = linear;
   using inplace = cat::str_inplace<16u>;

   // Quotes are stripped and `fmt("{:?}")` escapes are resolved.
   cat::verify(cat::scan_str<inplace>(R"("a, b")").verify() == "a, b");
   cat::verify(cat::scan_str<inplace>(R"("")").verify() == "");
   cat::verify(
      cat::scan_str<inplace>(R"("q\"\\\n\r\t\x")").verify() == "q\"\\\n\r\tx"
   );

   // Without a matching pair of quotes, the input is taken as is.
   cat::verify(cat::scan_str<inplace>(R"(")").verify() == "\"");
   cat::verify(cat::scan_str<inplace>(R"("ab)").verify() == "\"ab");
   cat::verify(cat::scan_str<inplace>(R"(a"b")").verify() == "a\"b\"");

   // Malformed quoted strings fail.
   cat::verify(cat::scan_str<inplace>(R"("a"b")").is_empty());
   cat::verify(cat::scan_str<inplace>(R"("ab\")").is_empty());

   cat::str_vec manual =
      cat::scan_str<cat::str_vec>(allocator, R"("x\ty")").verify();
   cat::verify(manual == "x\ty");
   manual.free(allocator);

   cat::raii::str_vec<> owning =
      cat::scan_str<cat::raii::str_vec<>>(allocator, R"("x\ty")").verify();
   cat::verify(owning == "x\ty");

   g_tests_alloc.free(page);
}
