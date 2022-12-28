#include <cat/format>
#include <cat/linear_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

TEST(test_format_strings) {
    // Initialize an allocator.
    cat::page_allocator pager;
    pager.reset();
    auto page = pager.opq_alloc_multi<cat::byte>(4_ki - 32).or_exit();
    defer(pager.free(page);)
    auto allocator =
        cat::linear_allocator::backed_handle(pager, page);

    // Test `int4` conversion.
    cat::string int_string = cat::to_chars(allocator, 10).or_exit();
    cat::verify(cat::compare_strings(int_string, "10"));
    cat::verify(int_string.size() == 3);

    // TODO: `constexpr` string comparison.
    // TODO: Test out of memory error handling.
    // TODO: Test formatting maximum value of integers.
    // TODO: Test `int1`, `uint1`, `int2`, `uint2`, `uint4`, `int8`, and
    // `uint8`.

    // TOOD: These stopped working for some reason.
    // constexpr cat::fixed_string const_int = cat::to_chars<136>();
    // constexpr cat::fixed_string const_negative = cat::to_chars<-1650>();

    // cat::verify(cat::compare_strings(const_int.data(), "136"));
    // cat::verify(cat::compare_strings(const_negative.data(), "-1650"));

    // Test formatting `int`.
    allocator.reset();
    cat::string formatted_string_int =
        cat::format(allocator, "bb{}aa{}cc", 52, 130).or_exit();
    // TODO: `formatted_string_int` has an incorrect `.size()`, but the content
    // is correct.
    cat::verify(cat::compare_strings(formatted_string_int, "bb52aa130cc"));
    // _ = cat::println(formatted_string_int);

    // Test formatting `float`.
    allocator.reset();
    cat::string string_float = cat::to_chars(allocator, 1.234f).or_exit();
    cat::verify(cat::compare_strings(string_float.data(), "1.234E0"),
                string_float);
    // _ = cat::println(string_float);

    cat::string formatted_string_float =
        cat::format(allocator, "a{}b", 1.234f).or_exit();
    cat::verify(cat::compare_strings(formatted_string_float, "a1.234E0b"));
    // _ = cat::println(formatted_string_float);

    cat::string formatted_string_double =
        cat::format(allocator, "a{}b", 1.234).or_exit();
    cat::verify(cat::compare_strings(formatted_string_double, "a1.234E0b"));
    // _ = cat::println(formatted_string_double);

    // Test `cat::to_string_at()`.
    cat::array<char, 100> array;
    cat::span<char> array_span{array.data(), array.size()};

    // TODO: This segfaults with optimizations enabled in GCC 13.
    // cat::string string_int_13 =
    //     cat::to_string_at(int4{13}, array_span).verify();
    // cat::verify(string_int_13.size() == 4);
    // cat::verify(cat::compare_strings(string_int_13.data(), "13"));

    // TODO: These stopped working for some reason.

    // cat::string string_neg_13 =
    //     cat::to_string_at(int4{-13}, array_span).verify();
    // cat::verify(string_neg_13.size() == 4);
    // cat::verify(cat::compare_strings(string_neg_13.data(), "-13"));

    // Test `cat::to_string_at()` in a `constexpr` context.
    // auto make_hi_in_const = [](int4 value) constexpr->cat::string {
    //     cat::array<char, 100> array{};
    //     cat::span<char> array_span{array.data(), array.size()};
    //     _ = cat::to_string_at(value, array_span).value();
    //     return "Hi";
    // };
    // [[maybe_unused]] constexpr auto hi = make_hi_in_const(1);
}
