#include <cat/format>
#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/pool_allocator>

#include "../unit_tests.hpp"

struct large_format_value {
   int value;
   char padding[32];

   large_format_value(int in_value) : value(in_value), padding{} {
   }

   large_format_value(large_format_value const&) = delete;
};

namespace cat {
template <typename CharT>
struct formatter<::large_format_value, CharT> : formatter_base<CharT> {
   auto
   format(::large_format_value const& value, format_context& context) const
      -> scaredy_format<void> {
      scaredy_format<void> result = context.append("large(");
      if (result.is_empty()) {
         return result;
      }
      result = formatter<int, CharT>{}.format(value.value, context);
      if (result.is_empty()) {
         return result;
      }
      return context.append(')');
   }
};
}  // namespace cat

$test(format_strings) {
   // Initialize an allocator.
   cat::span page = pager.alloc_multi<cat::byte>(4_uki).or_exit();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   // Test `int4` conversion.
   cat::str_view int_string = cat::to_chars(allocator, 10).or_exit();
   cat::verify(int_string == "10");
   cat::verify(int_string.size() == 2);

   // TODO: `constexpr` string comparison.
   // TODO: Test out of memory error handling.
   // TODO: Test formatting maximum value of integers.
   // TODO: Test `int1`, `uint1`, `int2`, `uint2`, `uint4`, `int8`, and
   // `uint8`.

   constexpr auto const_int = cat::to_chars<136>();
   constexpr auto const_negative = cat::to_chars<-1'650>();
   static_assert(const_int == "136");
   static_assert(const_negative == "-1650");

   // Test formatting `int`.
   allocator.reset();
   cat::str_view formatted_string_int =
      cat::fmt(allocator, "bb{}aa{}cc", 52, 130).or_exit();
   // TODO: `formatted_string_int` has an incorrect `.size()`, but the content
   // is correct.
   cat::verify(formatted_string_int == "bb52aa130cc");
   // cat::println(formatted_string_int);

   // Test formatting `float`.
   allocator.reset();
   cat::str_view string_float = cat::to_chars(allocator, 1.234f).or_exit();
   cat::verify(string_float == "1.234E0", string_float);
   // cat::println(string_float);

   cat::str_view formatted_string_float =
      cat::fmt(allocator, "a{}b", 1.234f).or_exit();
   cat::verify(formatted_string_float == "a1.234E0b");
   // cat::println(formatted_string_float);

   cat::str_view formatted_string_double =
      cat::fmt(allocator, "a{}b", 1.234).or_exit();
   cat::verify(formatted_string_double == "a1.234E0b");
   // cat::println(formatted_string_double);

   // Test `cat::to_string_at()`.
   cat::array<char, 100u> array;
   cat::span<char> array_span{array.data(), array.size()};

   // TODO: This segfaulted with optimizations enabled.
   // cat::string string_int_13 =
   //     cat::to_string_at(int4{13}, array_span).verify();
   // cat::verify(string_int_13.size() == 4);
   // cat::verify(string_int_13 == "13");

   // TODO: These stopped working for some reason.

   // cat::string string_neg_13 =
   //     cat::to_string_at(int4{-13}, array_span).verify();
   // cat::verify(string_neg_13.size() == 4);
   // cat::verify(string_neg_13 == "-13");

   // Test `cat::to_string_at()` in a `constexpr` context.
   // auto make_hi_in_const = [](int4 value) constexpr->cat::string {
   //     cat::array<char, 100> array{};
   //     cat::span<char> array_span{array.data(), array.size()};
   //     auto _ = cat::to_string_at(value, array_span).value();
   //     return "Hi";
   // };
   // [[maybe_unused]] constexpr auto hi = make_hi_in_const(1);
}

$test(fmt_long_pattern_substitutes) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).or_exit();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::str_view const formatted =
      cat::fmt(
         allocator,
         "{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:"
         "{}:{}:{}:{}:{}:{}:{}:{}",
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
         20, 21, 22, 23
      )
         .or_exit();

   cat::verify(
      (formatted
       == "0:1:2:3:4:5:6:7:8:9:10:11:12:13:14:15:16:17:18:19:20:21:22:23")
   );
}

$test(fmt_growth_failure) {
   // The arena holds the initial buffer but is too small to reallocate it
   // large enough for all 20 digits, so appending the number fails.
   cat::span storage = pager.alloc_multi<cat::byte>(24u).or_exit();
   $defer {
      pager.free(storage);
   };
   auto allocator = make_linear_allocator(storage);

   auto result = cat::fmt(allocator, "{}", uint8::max());
   cat::verify(result.is_empty());
}
