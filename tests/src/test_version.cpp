#include <cat/allocator_linear>
#include <cat/format>
#include <cat/version>

#include "../unit_tests.hpp"

namespace {

static_assert(cat::is_same<
              decltype(cat::version{1, 2, "rc.1"}),
              cat::version<int, int, cat::str_literal<4u>>>);

static_assert(cat::version{6, 5} < cat::version{6, 17});
static_assert(cat::version{6, 17} >= cat::version{6, 5});
static_assert(cat::version{1, 0, 0} > cat::version{0, 9, 9});
static_assert(cat::version{1, "abc"} == cat::version{1, "abc"});
static_assert(cat::version{1, "abc"} != cat::version{1, "abd"});

static_assert(cat::version{1, "rc.1"} < cat::version{1, "rc.2"});

// `version` can be a template argument.
template <cat::version value>
constexpr auto version_constant = value;

static_assert(
   version_constant<cat::version{1, "rc.1"}> == cat::version{1, "rc.1"}
);

// `str_view` fields have no ordering.
template <typename T>
concept is_ordered = requires(T value) { value < value; };

static_assert(is_ordered<cat::version<int4, int4>>);
static_assert(!is_ordered<cat::version<int4, cat::str_view>>);

constexpr auto
sum_fields() -> int {
   auto [major, minor, patch] = cat::version{1, 2, 3};
   return major + minor + patch;
}

static_assert(sum_fields() == 6);

// `version` inherits `tuple`'s subscript.
static_assert(cat::version{6, 17}[1u] == 17);
static_assert(cat::version{6, "rc.1"}[1u] == "rc.1");

// Other character types also become `str_literal`s.
static_assert(cat::is_same<
              decltype(cat::version{1, u8"rc"}),
              cat::version<int, cat::u8str_literal<2u>>>);

using kernel_version = cat::version<uint4, uint4, uint4>;

// Leading integer fields are named.
template <typename T>
concept has_minor = requires(T value) { value.minor(); };

template <typename T>
concept has_patch = requires(T value) { value.patch(); };

static_assert(kernel_version{6u, 17u, 3u}.major() == 6u);
static_assert(kernel_version{6u, 17u, 3u}.minor() == 17u);
static_assert(kernel_version{6u, 17u, 3u}.patch() == 3u);
static_assert(
   cat::is_same<decltype(cat::declval<kernel_version&>().major()), uint4&>
);
static_assert(
   cat::is_same<
      decltype(cat::declval<kernel_version const&>().patch()), uint4 const&>
);
static_assert(has_minor<cat::version<int, int>>);
static_assert(!has_patch<cat::version<int, int>>);
static_assert(has_patch<cat::version<int, int, int, cat::str_view>>);
static_assert(!has_patch<cat::version<int, int, cat::str_view>>);
static_assert(!has_minor<cat::version<int, cat::str_view>>);
static_assert(!has_minor<cat::version<int>>);

constexpr auto
bump_minor() -> uint4 {
   kernel_version kernel{6u, 17u, 3u};
   ++kernel.minor();
   return kernel.minor();
}

static_assert(bump_minor() == 18u);

}  // namespace

$test(version_format) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   auto allocator = make_allocator_linear(page);

   cat::verify(
      cat::fmt(allocator, "{}", cat::version{1, 2, 3}).verify() == "1.2.3"
   );
   cat::verify(
      cat::fmt(allocator, "{:..-+}", cat::version{1, 2, 3, "rc.1", "c0decafe"})
         .verify()
      == "1.2.3-rc.1+c0decafe"
   );
   // Unspelled separators are `.`.
   cat::verify(
      cat::fmt(allocator, "{:-}", cat::version{1, 2, 3}).verify() == "1-2.3"
   );
   cat::verify(cat::fmt(allocator, "{}", cat::version{7}).verify() == "7");
   // There are only two separators here.
   cat::verify(cat::fmt(allocator, "{:...}", cat::version{1, 2, 3}).is_empty());
}
