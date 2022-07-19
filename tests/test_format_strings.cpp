#include <cat/format>
#include <cat/linear_allocator>
#include <cat/page_allocator>

auto main() -> int {
    // Make allocator for string formatting.
    cat::PageAllocator pager;
    void* p_page = pager.p_alloc_multi(4_ki).or_exit();
    cat::LinearAllocator allocator{p_page, 4_ki - 32};

    // Test `int4` conversion.
    cat::String int_string = cat::to_string(allocator, 10).or_exit();
    Result(cat::compare_strings(int_string, "10")).or_exit();
    Result(int_string.size() == 3).or_exit();

    // TODO: Test out of memory error handling.

    constexpr cat::StaticString const_int = cat::to_string<136>();
    constexpr cat::StaticString const_negative = cat::to_string<-1650>();
    // TODO: `constexpr` string comparison.
    Result(cat::compare_strings(const_int.p_data(), "136")).or_exit();
    Result(cat::compare_strings(const_negative.p_data(), "-1650")).or_exit();

    allocator.reset();
    cat::String formatted_string =
        cat::format(allocator, "bb{}aa{}cc", 52, 130).or_exit();
    _ = cat::println(formatted_string);
    // TODO: `formatted_string` has an incorrect `.size()`, but the content is
    // correct.
    Result(cat::compare_strings(formatted_string, "bb52aa130cc")).or_exit();

    // Test `cat::to_string_at()`.
    cat::Array<char, 100> array;
    cat::Span<char> array_span{array.p_data(), array.size()};
    cat::String string_int_13 = cat::to_string_at(int4{13}, array_span).value();
    Result(cat::compare_strings(string_int_13.p_data(), "13")).or_exit();
    cat::String string_neg_13 =
        cat::to_string_at(int4{-13}, array_span).value();
    Result(cat::compare_strings(string_neg_13.p_data(), "-13")).or_exit();

    // Test `cat::to_string_at()` in a `constexpr` context.
    auto make_hi_in_const = [](int4 value) constexpr->cat::String {
        cat::Array<char, 100> array{};
        cat::Span<char> array_span{array.p_data(), array.size()};
        _ = cat::to_string_at(value, array_span).value();
        return "Hi";
    };
    [[maybe_unused]] constexpr auto hi = make_hi_in_const(1);

    cat::exit();
}
