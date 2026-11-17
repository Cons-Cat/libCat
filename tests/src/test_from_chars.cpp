#include <cat/array>
#include <cat/from_chars>
#include <cat/linux>
#include <cat/splitmix>
#include <cat/string>
#include <cat/tuple>
#include <cat/type_list>

#include "../unit_tests.hpp"
#include "integer_bounds.hpp"

namespace {

// The previous digit-at-a-time `from_chars()`, as a reference for the SWAR one.
template <typename T>
constexpr auto
reference_from_chars(cat::str_view input, T& value) -> cat::maybe<cat::idx> {
   // The overflow builtins only operate on raw integers.
   using raw_type = cat::raw_arithmetic_type<T>;
   cat::idx position = 0u;
   bool negative = false;
   if constexpr (cat::is_signed<T>) {
      if (input.size() > 0u && input[0u] == '-') {
         negative = true;
         position = 1u;
      }
   }
   cat::idx const digits_begin = position;
   raw_type result = 0;
   for (; position < input.size(); ++position) {
      char const character = input[position];
      if (character < '0' || character > '9') {
         break;
      }
      raw_type const digit = character - '0';
      if (__builtin_mul_overflow(result, 10, &result)) {
         return cat::nullopt;
      }
      bool const overflowed =
         negative ? __builtin_sub_overflow(result, digit, &result)
                  : __builtin_add_overflow(result, digit, &result);
      if (overflowed) {
         return cat::nullopt;
      }
   }
   if (position == digits_begin) {
      return cat::nullopt;
   }
   if (result > cat::limits<T>::max() || result < cat::limits<T>::min()) {
      return cat::nullopt;
   }
   value = result;
   return position;
}

// `from_chars()` takes a different path in constant evaluation, so these all
// run at compile time and at runtime.

// Parse all of `input` as `T`.
template <typename T>
constexpr auto
from_chars_whole(cat::str_view input) -> cat::maybe<T> {
   auto const [value, consumed] = $prop(cat::from_chars<T>(input));
   if (consumed != input.size()) {
      return cat::nullopt;
   }
   return value;
}

template <typename T>
constexpr void
verify_from_chars_integer(integer_bounds bounds = bounds_of<T>()) {
   cat::verify(from_chars_whole<T>("0").verify() == 0);
   cat::verify(from_chars_whole<T>("42").verify() == 42);
   if constexpr (cat::is_signed<T>) {
      cat::verify(from_chars_whole<T>("-42").verify() == -42);
   } else {
      cat::verify(cat::from_chars<T>("-42").is_empty());
   }
   cat::verify(
      from_chars_whole<T>(bounds.min).verify() == cat::limits<T>::min()
   );
   cat::verify(
      from_chars_whole<T>(bounds.max).verify() == cat::limits<T>::max()
   );
   cat::verify(cat::from_chars<T>(bounds.below_min).is_empty());
   cat::verify(cat::from_chars<T>(bounds.above_max).is_empty());
}

constexpr auto
verify_constexpr_from_chars_integers() -> bool {
   []<typename... Ts>(cat::type_list<Ts...>) {
      (verify_from_chars_integer<Ts>(), ...);
   }(parseable_integers{});

   verify_from_chars_integer<cat::idx>(idx_bounds);
   return true;
}

constexpr auto
verify_constexpr_from_chars_malformed() -> bool {
   cat::verify(cat::from_chars<int4>("").is_empty());
   cat::verify(cat::from_chars<int4>("-").is_empty());
   cat::verify(cat::from_chars<int4>("+1").is_empty());
   cat::verify(cat::from_chars<int4>(" 1").is_empty());
   cat::verify(cat::from_chars<int4>("x1").is_empty());
   cat::verify(cat::from_chars<int4>("--1").is_empty());
   cat::verify(cat::from_chars<uint4>("-0").is_empty());

   // Only a prefix is consumed.
   auto const [value, consumed] = cat::from_chars<int4>("42 rest").verify();
   cat::verify(value == 42);
   cat::verify(consumed == 2u);
   auto const [negative, negative_consumed] =
      cat::from_chars<int4>("-2147483648 rest").verify();
   cat::verify(negative == cat::limits<int4>::min());
   cat::verify(negative_consumed == 11u);
   auto const [index, index_consumed] =
      cat::from_chars<cat::idx>("12a").verify();
   cat::verify(index == 12u);
   cat::verify(index_consumed == 2u);

   auto const [small, small_consumed] =
      cat::from_chars<int1>("00000000000000000000000000000000000042").verify();
   cat::verify(small == 42);
   cat::verify(small_consumed == 38u);
   auto const [big, big_consumed] =
      cat::from_chars<uint8>("000000000018446744073709551615").verify();
   cat::verify(big == cat::limits<uint8>::max());
   cat::verify(big_consumed == 30u);
   auto const [zero, zero_consumed] = cat::from_chars<int1>("-0").verify();
   cat::verify(zero == 0);
   cat::verify(zero_consumed == 2u);
   return true;
}

constexpr auto
verify_constexpr_from_chars_non_digits() -> bool {
   // A non-digit at each position of the first two chunks ends the number.
   for (cat::idx i = 0u; i < 16u; ++i) {
      cat::array<char, 16u> string = cat::make_array_filled<16u>('1');
      string[i] = 'x';
      cat::maybe_from_chars<uint8> const result =
         cat::from_chars<uint8>(cat::str_view(string.data(), 16u));
      if (i == 0u) {
         cat::verify(result.is_empty());
      } else {
         auto const& [value, consumed] = result.value();
         cat::verify(consumed == i);
      }
   }

   // Bytes next to `'0'` and `'9'`, and bytes that make the SWAR math carry.
   constexpr cat::array<char, 8u> non_digits = {
      '/', ':', '\x00', '\x7f', '\x80', '\xb9', '\xba', '\xff',
   };
   for (char const non_digit : non_digits) {
      // In the eight-digit loop, and in the tail.
      constexpr cat::array<cat::idx, 2u> positions = {3u, 11u};
      for (cat::idx const position : positions) {
         cat::array<char, 16u> string = cat::make_array_filled<16u>('1');
         string[position] = non_digit;
         auto const [value, consumed] =
            cat::from_chars<uint8>(cat::str_view(string.data(), 16u)).verify();
         cat::verify(consumed == position);
      }
   }
   return true;
}

// Every count of digits, with more digits after the end of the `str_view`.
constexpr auto
verify_constexpr_from_chars_lengths() -> bool {
   constexpr cat::str_view digits = "1111111111111111111111111111111111111111"
                                    "2222222222";
   cat::wrap_uint16 expected = 0u;
   for (cat::idx length = 1u; length <= 40u; ++length) {
      expected = expected * 10u + 1u;
      cat::str_view const input(digits.data(), length);
      cat::maybe_from_chars<uint16> const result =
         cat::from_chars<uint16>(input);
      // 40 `'1'`s overflow a `uint16`.
      if (length == 40u) {
         cat::verify(result.is_empty());
      } else {
         auto const& [value, consumed] = result.value();
         cat::verify(value == uint16(expected));
         cat::verify(consumed == length);
      }
   }
   return true;
}

// The random strings are mostly digits, so we get long numbers.
template <typename T>
constexpr void
verify_from_chars_matches_reference(
   cat::splitmix64_engine& engine, cat::idx iterations
) {
   constexpr cat::str_view alphabet = "0123456789012345678901234567890-x:/\xff";
   for (cat::idx iteration = 0u; iteration < iterations; ++iteration) {
      cat::array<char, 48u> string;
      cat::idx const length(engine(48u));
      for (cat::idx i = 0u; i < length; ++i) {
         cat::idx const index(engine(alphabet.size()));
         string[i] = alphabet[index];
      }
      cat::str_view const input(string.data(), length);
      T reference_value = 0;
      cat::maybe_from_chars<T> const result = cat::from_chars<T>(input);
      cat::maybe<cat::idx> const reference_consumed =
         reference_from_chars(input, reference_value);
      cat::verify(result.has_value() == reference_consumed.has_value());
      if (result.has_value()) {
         auto const& [value, consumed] = result.value();
         cat::verify(consumed == reference_consumed.value());
         cat::verify(value == reference_value);
      }
   }
}

template <typename T>
constexpr auto
verify_constexpr_from_chars_reference(cat::idx iterations) -> bool {
   cat::splitmix64_engine engine(42u);
   verify_from_chars_matches_reference<T>(engine, iterations);
   return true;
}

// This should fit in two registers.
static_assert(
   sizeof(cat::maybe_from_chars<int8>) == sizeof(cat::tuple<int8, cat::idx>)
);

static_assert(verify_constexpr_from_chars_integers());
static_assert(verify_constexpr_from_chars_malformed());
static_assert(verify_constexpr_from_chars_non_digits());
static_assert(verify_constexpr_from_chars_lengths());
// Constant evaluation runs out of steps at about 100 strings per type.
static_assert(verify_constexpr_from_chars_reference<int1>(50u));
static_assert(verify_constexpr_from_chars_reference<uint2>(50u));
static_assert(verify_constexpr_from_chars_reference<int4>(50u));
static_assert(verify_constexpr_from_chars_reference<int8>(50u));
static_assert(verify_constexpr_from_chars_reference<uint8>(50u));
static_assert(verify_constexpr_from_chars_reference<cat::idx>(50u));
static_assert(verify_constexpr_from_chars_reference<int16>(50u));
static_assert(verify_constexpr_from_chars_reference<uint16>(50u));

}  // namespace

$test(from_chars_runtime) {
   verify_constexpr_from_chars_integers();
   verify_constexpr_from_chars_malformed();
   verify_constexpr_from_chars_non_digits();
   verify_constexpr_from_chars_lengths();
   verify_constexpr_from_chars_reference<int1>(4'000u);
   verify_constexpr_from_chars_reference<uint2>(4'000u);
   verify_constexpr_from_chars_reference<int4>(4'000u);
   verify_constexpr_from_chars_reference<int8>(4'000u);
   verify_constexpr_from_chars_reference<uint8>(4'000u);
   verify_constexpr_from_chars_reference<cat::idx>(4'000u);
   verify_constexpr_from_chars_reference<int16>(4'000u);
   verify_constexpr_from_chars_reference<uint16>(4'000u);
}

$test(from_chars_page_boundary) {
   // Reading past the first page segfaults.
   cat::byte* p_pages =
      nix::sys_mmap(
         nullptr, 2u * cat::page_size, nix::memory_protection_flags::read_write,
         nix::memory_flags::privately | nix::memory_flags::anonymous,
         nix::invalid_file_descriptor, 0
      )
         .verify();
   nix::sys_mprotect(
      p_pages + cat::page_size, cat::page_size,
      nix::memory_protection_flags::none
   )
      .verify();
   char* p_page_end = __builtin_bit_cast(char*, p_pages + cat::page_size);

   // The digits after each number are outside of the `str_view`, so they must
   // not be parsed.
   for (cat::idx length = 1u; length <= 20u; ++length) {
      for (cat::idx gap = 0u; gap < 10u; ++gap) {
         char* p_number = p_page_end - gap - length;
         uint8 expected = 0u;
         for (cat::idx i = 0u; i < length; ++i) {
            p_number[i] = '1';
            expected = expected * 10u + 1u;
         }
         for (cat::idx i = 0u; i < gap; ++i) {
            p_number[length + i] = '2';
         }
         auto const [value, consumed] =
            cat::from_chars<uint8>(cat::str_view(p_number, length)).verify();
         cat::verify(value == expected);
         cat::verify(consumed == length);
      }
   }

   nix::sys_munmap(p_pages, 2u * cat::page_size).verify();
}

$test(from_chars_asan_redzone) {
   // ASan shouldn't catch `from_chars()` reading past these arrays.
   cat::array<char, 3u> const three_digits = {'1', '2', '3'};
   auto const [three, three_consumed] =
      cat::from_chars<int4>(cat::str_view(three_digits.data(), 3u)).verify();
   cat::verify(three == 123);
   cat::verify(three_consumed == 3u);

   cat::array<char, 9u> const nine_digits = {
      '9', '8', '7', '6', '5', '4', '3', '2', '1',
   };
   auto const [nine, nine_consumed] =
      cat::from_chars<int4>(cat::str_view(nine_digits.data(), 9u)).verify();
   cat::verify(nine == 987'654'321);
   cat::verify(nine_consumed == 9u);
}
