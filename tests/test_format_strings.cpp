#include <cat/format>
#include <cat/linear_allocator>
#include <cat/page_allocator>

auto main() -> int {
    // Make allocator for string formatting.
    cat::PageAllocator pager;
    cat::Byte* p_page = pager.p_alloc_multi<cat::Byte>(4_ki).or_exit();
    cat::LinearAllocator allocator{p_page, 4_ki - 32};

    // Test `int4` conversion.
    cat::String int_string = cat::to_chars(allocator, 10).or_exit();
    cat::verify(cat::compare_strings(int_string, "10"));
    cat::verify(int_string.size() == 3);

    // TODO: Test out of memory error handling.
    // TODO: Test formatting maximum value of integers.
    // TODO: Test `int1`, `uint1`, `int2`, `uint2`, `uint4`, `int8`, and
    // `uint8`.

    constexpr cat::StaticString const_int = cat::to_chars<136>();
    constexpr cat::StaticString const_negative = cat::to_chars<-1650>();
    // TODO: `constexpr` string comparison.
    cat::verify(cat::compare_strings(const_int.p_data(), "136"));
    cat::verify(cat::compare_strings(const_negative.p_data(), "-1650"));

    // Test formatting `int`.
    allocator.reset();
    cat::String formatted_string_int =
        cat::format(allocator, "bb{}aa{}cc", 52, 130).or_exit();
    _ = cat::println(formatted_string_int);
    // TODO: `formatted_string_int` has an incorrect `.size()`, but the content
    // is correct.
    cat::verify(cat::compare_strings(formatted_string_int, "bb52aa130cc"));

    // Test formatting `float`.
    allocator.reset();
    cat::String string_float = cat::to_chars(allocator, 1.234f).or_exit();
    _ = cat::println(string_float);

    cat::String formatted_string_float =
        cat::format(allocator, "a{}b", 1.234f).or_exit();
    _ = cat::println(formatted_string_float);
    cat::verify(cat::compare_strings(formatted_string_float, "a1.234E0b"));

    cat::String formatted_string_double =
        cat::format(allocator, "a{}b", 1.234).or_exit();
    _ = cat::println(formatted_string_double);
    cat::verify(cat::compare_strings(formatted_string_double, "a1.234E0b"));

    // Test `cat::to_string_at()`.
    cat::Array<char, 100> array;
    cat::Span<char> array_span{array.p_data(), array.size()};
    cat::String string_int_13 = cat::to_string_at(int4{13}, array_span).value();
    cat::verify(cat::compare_strings(string_int_13.p_data(), "13"));
    cat::String string_neg_13 =
        cat::to_string_at(int4{-13}, array_span).value();
    cat::verify(cat::compare_strings(string_neg_13.p_data(), "-13"));

    // Test `cat::to_string_at()` in a `constexpr` context.
    auto make_hi_in_const = [](int4 value) constexpr->cat::String {
        cat::Array<char, 100> array{};
        cat::Span<char> array_span{array.p_data(), array.size()};
        _ = cat::to_string_at(value, array_span).value();
        return "Hi";
    };
    [[maybe_unused]] constexpr auto hi = make_hi_in_const(1);
}
