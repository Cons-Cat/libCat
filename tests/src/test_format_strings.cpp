#include <cat/atomic>
#include <cat/bitset>
#include <cat/format>
#include <cat/linear_allocator>
#include <cat/list>
#include <cat/page_allocator>
#include <cat/pool_allocator>
#include <cat/random>
#include <cat/simd>
#include <cat/thread>
#include <cat/vec>

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
   cat::span page = pager.alloc_multi<cat::byte>(4_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   // Test `int4` conversion.
   cat::str_view int_string = cat::to_chars(allocator, 10).verify();
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
      cat::fmt(allocator, "bb{}aa{}cc", 52, 130).verify();
   // TODO: `formatted_string_int` has an incorrect `.size()`, but the content
   // is correct.
   cat::verify(formatted_string_int == "bb52aa130cc");
   // cat::println(formatted_string_int);

   // Test formatting `float`.
   allocator.reset();
   cat::str_view string_float = cat::to_chars(allocator, 1.234f).verify();
   cat::verify(string_float == "1.234E0", string_float);
   // cat::println(string_float);

   cat::str_view formatted_string_float =
      cat::fmt(allocator, "a{}b", 1.234f).verify();
   cat::verify(formatted_string_float == "a1.234E0b");
   // cat::println(formatted_string_float);

   cat::str_view formatted_string_double =
      cat::fmt(allocator, "a{}b", 1.234).verify();
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

$test(format_classes) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::array<int, 3u> numbers{1, 2, 3};
   cat::verify(cat::fmt(allocator, "{}", numbers).verify() == "[1, 2, 3]");

   allocator.reset();
   cat::array<cat::str_view, 2u> words{"cat", "dog"};
   cat::verify(
      cat::fmt(allocator, "{}", words).verify() == R"(["cat", "dog"])"
   );

   auto allocator_ref = cat::allocator_ref<cat::linear_allocator>(allocator);

   allocator.reset();
   {
      cat::vec<int> empty_vec;
      cat::verify(cat::fmt(allocator, "{}", empty_vec).verify() == "[]");
   }

   allocator.reset();
   {
      cat::vec<int> vec_numbers;
      vec_numbers.push_back(allocator_ref, 1).verify();
      vec_numbers.push_back(allocator_ref, 2).verify();
      vec_numbers.push_back(allocator_ref, 3).verify();
      cat::verify(
         cat::fmt(allocator, "{}", vec_numbers).verify() == "[1, 2, 3]"
      );
   }

   allocator.reset();
   {
      cat::vec<cat::str_view> vec_words;
      vec_words.push_back(allocator_ref, cat::str_view("cat")).verify();
      vec_words.push_back(allocator_ref, cat::str_view("dog")).verify();
      cat::verify(
         cat::fmt(allocator, "{}", vec_words).verify() == R"(["cat", "dog"])"
      );
   }

   allocator.reset();
   {
      cat::list<int> empty_list;
      cat::verify(cat::fmt(allocator, "{}", empty_list).verify() == "[]");
   }

   allocator.reset();
   {
      cat::list<int> list_numbers;
      list_numbers.push_back(allocator_ref, 1).verify();
      list_numbers.push_back(allocator_ref, 2).verify();
      list_numbers.push_back(allocator_ref, 3).verify();
      cat::verify(
         cat::fmt(allocator, "{}", list_numbers).verify() == "[1, 2, 3]"
      );
   }

   allocator.reset();
   {
      cat::list<cat::str_view> list_words;
      list_words.push_back(allocator_ref, cat::str_view("cat")).verify();
      list_words.push_back(allocator_ref, cat::str_view("dog")).verify();
      cat::verify(
         cat::fmt(allocator, "{}", list_words).verify() == R"(["cat", "dog"])"
      );
   }
   allocator.reset();
   {
      cat::str_vec empty_str_vec;
      cat::verify(cat::fmt(allocator, "{}", empty_str_vec).verify() == "");
      cat::verify(
         cat::fmt(allocator, "{:?}", empty_str_vec).verify() == R"("")"
      );
   }

   allocator.reset();
   {
      cat::str_vec text =
         cat::make_str_vec(allocator_ref, cat::str_view("cat")).verify();
      cat::verify(cat::fmt(allocator, "{}", text).verify() == "cat");
      cat::verify(cat::fmt(allocator, "{:?}", text).verify() == R"("cat")");
   }

   allocator.reset();
   {
      cat::zstr_vec text =
         cat::make_zstr_vec(allocator_ref, cat::str_view("cat")).verify();
      cat::verify(cat::fmt(allocator, "{}", text).verify() == "cat");
      cat::verify(cat::fmt(allocator, "{:?}", text).verify() == R"("cat")");
   }

   allocator.reset();
   {
      cat::raii::str_vec<cat::linear_allocator> text =
         cat::raii::make_str_vec(allocator_ref, cat::str_view("cat")).verify();
      cat::verify(cat::fmt(allocator, "{}", text).verify() == "cat");
      cat::verify(cat::fmt(allocator, "{:?}", text).verify() == R"("cat")");
   }

   allocator.reset();
   cat::str_inplace<8u> inplace = cat::make_str_inplace<8u>("cat");
   cat::verify(cat::fmt(allocator, "{}", inplace).verify() == "cat");
   cat::verify(cat::fmt(allocator, "{:?}", inplace).verify() == R"("cat")");

   allocator.reset();
   cat::zstr_inplace<8u> zinplace = cat::make_zstr_inplace<8u>("cat");
   cat::verify(cat::fmt(allocator, "{}", zinplace).verify() == "cat");
   cat::verify(cat::fmt(allocator, "{:?}", zinplace).verify() == R"("cat")");

   allocator.reset();
   auto values = cat::make_tuple(1, cat::str_view("two"), true);
   cat::verify(
      cat::fmt(allocator, "{}", values).verify() == R"((1, "two", true))"
   );

   allocator.reset();
   cat::maybe<int> some = 7;
   cat::maybe<int> none;
   cat::verify(cat::fmt(allocator, "{} {}", some, none).verify() == "[7] []");

   allocator.reset();
   cat::variant<int, cat::str_view> active = cat::str_view("value");
   cat::variant<int, cat::str_view> empty;
   cat::verify(
      cat::fmt(allocator, "{} {}", active, empty).verify() == "value <empty>"
   );

   allocator.reset();
   large_format_value large(42);
   cat::verify(
      cat::fmt(allocator, "{} {} {}", large, 'x', "text").verify()
      == "large(42) x text"
   );

   allocator.reset();
   cat::verify(cat::fmt(allocator, "{:}", large).verify() == "large(42)");

   allocator.reset();
   cat::maybe<void> engaged_void = cat::monostate;
   cat::maybe<void> empty_void;
   cat::verify(
      cat::fmt(allocator, "{} {}", engaged_void, empty_void).verify()
      == "[void] []"
   );

   allocator.reset();
   int* p_null = nullptr;
   cat::verify(cat::fmt(allocator, "{}", p_null).verify() == "0x0");
}

$test(fmt_specs_and_braces) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::verify(cat::fmt(allocator, "a{:}b", 7).verify() == "a7b");

   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", "cat").verify() == "cat");

   allocator.reset();
   cat::verify(cat::fmt(allocator, "{:?}", "cat").verify() == R"("cat")");

   allocator.reset();
   cat::verify(cat::fmt(allocator, "{:?}", 'x').verify() == "'x'");

   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", 'x').verify() == "x");

   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{:?}", R"(a"b
)")
         .verify()
      == R"("a\"b\n")"
   );

   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{:?}", R"(xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"yy)")
         .verify()
      == R"("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"yy")"
   );

   allocator.reset();
   cat::verify(
      cat::fmt(
         allocator, "{:?}",
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      )
         .verify()
      == R"("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")"
   );

   allocator.reset();
   auto unknown_debug_int = cat::fmt(allocator, "{:?}", 1);
   cat::verify(unknown_debug_int.is_empty());
   cat::verify(
      unknown_debug_int.error() == cat::format_errors::unknown_format_specifier
   );

   allocator.reset();
   cat::verify(cat::fmt(allocator, "x{{y}}z").verify() == "x{y}z");

   allocator.reset();
   cat::verify(cat::fmt(allocator, "{{{}{{}}}}", 1).verify() == "{1{}}");

   allocator.reset();
   auto unknown = cat::fmt(allocator, "{:x}", 1);
   cat::verify(unknown.is_empty());
   cat::verify(unknown.error() == cat::format_errors::unknown_format_specifier);

   allocator.reset();
   auto unmatched_open = cat::fmt(allocator, "{", 1);
   cat::verify(unmatched_open.is_empty());
   cat::verify(
      unmatched_open.error() == cat::format_errors::unmatched_open_brace
   );

   allocator.reset();
   auto unmatched_close = cat::fmt(allocator, "}", 1);
   cat::verify(unmatched_close.is_empty());
   cat::verify(
      unmatched_close.error() == cat::format_errors::unmatched_open_brace
   );

   allocator.reset();
   auto missing_arg = cat::fmt(allocator, "{} {}", 1);
   cat::verify(missing_arg.is_empty());
   cat::verify(
      missing_arg.error() == cat::format_errors::argument_type_mismatch
   );

   allocator.reset();
   auto extra_arg = cat::fmt(allocator, "{}", 1, 2);
   cat::verify(extra_arg.is_empty());
   cat::verify(extra_arg.error() == cat::format_errors::argument_type_mismatch);

   allocator.reset();
   large_format_value large(9);
   auto bad_custom = cat::fmt(allocator, "{:d}", large);
   cat::verify(bad_custom.is_empty());
   cat::verify(
      bad_custom.error() == cat::format_errors::unknown_format_specifier
   );
}

$test(fmt_long_pattern_substitutes) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
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
         .verify();

   cat::verify(
      (formatted
       == "0:1:2:3:4:5:6:7:8:9:10:11:12:13:14:15:16:17:18:19:20:21:22:23")
   );
}

$test(fmt_growth_failure) {
   // The arena holds the initial buffer but is too small to reallocate it
   // large enough for all 20 digits, so appending the number fails.
   cat::span storage = pager.alloc_multi<cat::byte>(24u).verify();
   $defer {
      pager.free(storage);
   };
   auto allocator = make_linear_allocator(storage);

   auto result = cat::fmt(allocator, "{}", uint8::max());
   cat::verify(result.is_empty());
}

$test(fmt_public_type_families) {
   cat::span page = pager.alloc_multi<cat::byte>(32_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::atomic<int4> atomic_value{int4{11}};
   cat::verify(
      cat::fmt(allocator, "{}", atomic_value).verify() == "atomic(11)"
   );

   allocator.reset();
   cat::atomic_flag flag;
   cat::verify(cat::fmt(allocator, "{}", flag).verify() == "flag(false)");
   auto _ = flag.test_and_set();
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", flag).verify() == "flag(true)");

   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{} {}", cat::infinity, -cat::infinity).verify()
      == "inf -inf"
   );

   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{}", cat::monostate).verify() == "monostate"
   );

   allocator.reset();
   cat::source_location const location = cat::source_location::current();
   cat::str_view const location_text =
      cat::fmt(allocator, "{}", location).verify();
   cat::str_view const location_expected =
      cat::fmt(
         allocator, "{}:{}:{}:{}", location.file_name(), location.line(),
         location.column(), location.function_name()
      )
         .verify();
   cat::verify(location_text == location_expected);

   allocator.reset();
   cat::scaredy<int4, uint4> scaredy_ok = int4{4};
   cat::scaredy<int4, uint4> scaredy_err = uint4{8};
   cat::verify(
      cat::fmt(allocator, "{} {}", scaredy_ok, scaredy_err).verify()
      == "ok(4) err(8)"
   );

   allocator.reset();
   cat::null_allocator null_alloc;
   cat::verify(
      cat::fmt(allocator, "{}", null_alloc).verify() == "allocator(0/0)"
   );

   allocator.reset();
   cat::str_view const linear_text =
      cat::fmt(allocator, "{}", allocator).verify();
   cat::verify(linear_text.size() >= 11u);
   cat::verify(linear_text[0] == 'a');
   cat::verify(linear_text[1] == 'l');
   cat::verify(linear_text[2] == 'l');
   cat::verify(linear_text[3] == 'o');
   cat::verify(linear_text[4] == 'c');
   cat::verify(linear_text[9] == '(');
   cat::verify(linear_text.find('/').has_value());

   allocator.reset();
   cat::manual::thread worker;
   cat::verify(cat::fmt(allocator, "{}", worker).verify() == "0");
   cat::verify(cat::fmt(allocator, "{}", worker.get_id()).verify() == "0");
   cat::thread::id const null_id;
   cat::verify(cat::fmt(allocator, "{}", null_id).verify() == "0");
   cat::str_view const this_id =
      cat::fmt(allocator, "{}", cat::this_thread::get_id()).verify();
   cat::verify(this_id != "0");

   allocator.reset();
   cat::uniform_int_distribution<int4> uniform_ints(2, 9);
   cat::verify(
      cat::fmt(allocator, "{}", uniform_ints).verify() == "(a=2, b=9)"
   );
   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{}", uniform_ints.param()).verify()
      == "param(a=2, b=9)"
   );

   allocator.reset();
   cat::bernoulli_distribution bernoulli(0.25);
   cat::str_view const bernoulli_text =
      cat::fmt(allocator, "{}", bernoulli).verify();
   cat::str_view const bernoulli_expected =
      cat::fmt(allocator, "(p={})", bernoulli.p()).verify();
   cat::verify(bernoulli_text == bernoulli_expected);

   allocator.reset();
   cat::normal_distribution normal(1.5, 0.5);
   cat::str_view const normal_text = cat::fmt(allocator, "{}", normal).verify();
   cat::str_view const normal_expected =
      cat::fmt(
         allocator, "(mean={}, stddev={})", normal.mean(), normal.stddev()
      )
         .verify();
   cat::verify(normal_text == normal_expected);

   allocator.reset();
   cat::poisson_distribution poisson(3.0);
   cat::str_view const poisson_text =
      cat::fmt(allocator, "{}", poisson).verify();
   cat::str_view const poisson_expected =
      cat::fmt(allocator, "(mean={})", poisson.mean()).verify();
   cat::verify(poisson_text == poisson_expected);

   allocator.reset();
   cat::seed_sequence seeds({1u, 2u, 3u});
   cat::verify(
      cat::fmt(allocator, "{}", seeds).verify() == "seed_sequence[1, 2, 3]"
   );
}

$test(fmt_bit_containers_and_proxies) {
   cat::span page = pager.alloc_multi<cat::byte>(32_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::bit_value set_bit = true;
   cat::bit_value clear_bit = false;
   cat::verify(
      cat::fmt(allocator, "{} {}", set_bit, clear_bit).verify() == "true false"
   );

   allocator.reset();
   cat::bitset<4u> bits{};
   bits[1] = true;
   bits[3] = true;
   cat::verify(
      cat::fmt(allocator, "{}", bits).verify() == "[false, true, false, true]"
   );

   allocator.reset();
   cat::array<cat::uint4, 1u> words{};
   cat::bit_span span_bits(words.data(), 0u, 4u);
   span_bits[0] = true;
   span_bits[2] = true;
   cat::verify(
      cat::fmt(allocator, "{}", span_bits).verify()
      == "[true, false, true, false]"
   );

   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{} {}", cat::bool2(true), cat::bool4(false)).verify()
      == "true false"
   );

   allocator.reset();
   cat::int4 number = 42;
   cat::verify(cat::fmt(allocator, "{}", number.wrap()).verify() == "42");

   allocator.reset();
   cat::float4 floating = 1.5f;
   cat::str_view const precise_text =
      cat::fmt(allocator, "{}", floating.precise()).verify();
   cat::str_view const precise_expected =
      cat::fmt(allocator, "{}", floating).verify();
   cat::verify(precise_text == precise_expected);

   allocator.reset();
   cat::int4 atomic_storage = 7;
   cat::atomic_ref_seq_cst<cat::int4> atomic_ref(atomic_storage);
   cat::verify(
      cat::fmt(allocator, "{}", atomic_ref).verify() == "atomic_ref(7)"
   );

   allocator.reset();
   cat::fixed_size_simd<cat::int4, 4u> lanes(1, 2, 3, 4);
   cat::verify(
      cat::fmt(allocator, "{}", lanes.wrap()).verify() == "[1, 2, 3, 4]"
   );
   allocator.reset();
   cat::fixed_size_simd<cat::float4, 2u> floats(1.25f, 2.5f);
   cat::str_view const simd_precise =
      cat::fmt(allocator, "{}", floats.precise()).verify();
   cat::str_view const simd_expected =
      cat::fmt(allocator, "{}", floats).verify();
   cat::verify(simd_precise == simd_expected);
}

$test(fmt_to_output_iterator) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::vec<char> output;
   cat::verify(
      cat::fmt_to(allocator, cat::as_back_inserter(output), "x{}y", 42)
         .has_value()
   );
   cat::verify(cat::str_view(output.data(), output.size()) == "x42y");

   // A second call continues where the first one stopped.
   cat::verify(
      cat::fmt_to(allocator, cat::as_back_inserter(output), "-{}", true)
         .has_value()
   );
   cat::verify(cat::str_view(output.data(), output.size()) == "x42y-true");
}

$test(fmt_to_streams_past_staging_buffer) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   // Longer than the fixed staging buffer, so it drains mid-format.
   cat::array<char, 300u> letters;
   for (cat::idx i; i < letters.size(); ++i) {
      letters[i] = 'z';
   }

   cat::vec<char> output;
   cat::verify(
      cat::fmt_to(
         allocator, cat::as_back_inserter(output), "[{}]",
         cat::str_view(letters.data(), letters.size())
      )
         .has_value()
   );
   cat::verify(output.size() == 302u);
   cat::verify(output[0u] == '[');
   cat::verify(output[1u] == 'z');
   cat::verify(output[300u] == 'z');
   cat::verify(output[301u] == ']');
}
