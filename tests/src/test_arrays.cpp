#include <cat/array>
#include <cat/math>
#include <cat/runtime>
#include <cat/string>
#include <cat/utility>

#include "../unit_tests.hpp"

struct move_only {
    int i = 0;
    move_only() = default;
    move_only(move_only const&) = delete;

    move_only(move_only&& other) : i(other.i) {
    }
};

TEST(test_arrays) {
    // TODO: This passes in GCC 15 but not in Clang 19.
#ifndef __clang__
    static_assert(cat::is_trivial<cat::array<int, 1u>>);
#endif

    // List-initializing a array:
    cat::array array_1{0, 1, 2, 3, 4};
    // List-assigning a array:
    array_1 = {5, 6, 7, 8, 9};
    // Default initializing a array:
    cat::array<int4, 1u> array_2;

    // Move constructing a array:
    [[maybe_unused]]
    cat::array array_move_only = {move_only()};
    // Move assigning a array:
    [[maybe_unused]]
    cat::array array_3 = mov array_move_only;

    // `const` array.
    cat::array<int4, 3u> const array_const = {0, 1, 2};
    [[maybe_unused]]
    int4 const_val = array_const.at(1).or_exit();

    // Repeat those tests in a constexpr context.
    [] consteval {
        cat::array<int4, 1u> const_array_1{};
        cat::array<int4, 1u> const_array_2 = {1};
        const_array_2 = const_array_1;
    }();

    // Test that the array is iterable.
    idx count;
    for (int& a : array_1) {
        cat::verify(a == array_1[count]);
        ++count;
    }

    for (int& a : cat::as_reverse(array_1)) {
        --count;
        cat::verify(a == array_1[count]);
    }

    cat::verify(array_1.front() == 5);
    cat::verify(array_1.back() == 9);

    count = 0u;
    for (int const& a : cat::as_const(array_1)) {
        cat::verify(a == array_1[count]);
        ++count;
    }
    auto _ = array_1.cbegin();

    for (int const& a : cat::as_const_reverse(array_1)) {
        --count;
        cat::verify(a == array_1[count]);
    }

    int4 array_to = *(array_1.begin().advance_to(--array_1.end()));
    cat::verify(array_to == array_1.back());

    // Index in and out of bounds.
    cat::verify(array_1.at(0).value() == 5);
    cat::verify(!array_1.at(6).has_value());

    // Deducing type.
    cat::array implicit_array_1 = {0, 1, 2, 3, 4};
    cat::array implicit_array_2{0, 1, 2, 3, 4};
    cat::array implicit_array_3(0, 1, 2, 3, 4);
    static_assert(implicit_array_1.size() == 5u);
    static_assert(implicit_array_1.capacity() == 5);
    auto _ = implicit_array_1.capacity();
    static_assert(implicit_array_2.size() == 5);
    static_assert(implicit_array_3.size() == 5);

    // Max elements.
    // constexpr cat::array array_4 = {0, 2, 8, 5};
    // constexpr int4 max_1 = cat::max(array_4);
    //         cat::verify(max_1 == 8);

    // int4 min_1 = cat::min(array_4);
    //         cat::verify(min_1 == 0);

    // TODO: string deduction:
    //     cat::array implicit_string = "Hi, Conscat!";
    // static_assert(implicit_string.size() ==
    //               cat::string_length("Hi, Conscat!"));

    // TODO: Test `constexpr`.

    // Slicing array.
    [[maybe_unused]]
    cat::span span = array_1.first(1u);
    auto _ = array_1.subspan(0u, 2u);
    auto _ = array_1.last(2u);

    [[maybe_unused]]
    cat::span const span_const = array_1.first(1u);
    auto _ = array_const.subspan(0u, 2u);
    auto _ = array_const.last(2u);

    // Test array copy-assignment.
    cat::array base_array = {0_i4, 0, 0, 0};
    cat::array copy_array = {1, 2, 3, 4};
    cat::array copy_converting_array = {int2{1}, 2, 3, 4};
    base_array = copy_array;
    base_array = copy_converting_array;
    // TODO: Test these properly.
    // cat::array move_only_array = {5, 6, 7, 8};
    // cat::array move_converting_array = {int2{5}, 6, 7, 8};
    // base_array = move(move_array);
    // base_array = move(move_converting_array);

    // Test array fill.
    cat::array filled_array = cat::make_array_filled<8>(6_i4);
    for (idx i; i < 8; ++i) {
        cat::verify(filled_array[i] == 6);
    }

    filled_array.fill(9);
    for (idx i; i < 8; ++i) {
        cat::verify(filled_array[i] == 9);
    }

    // Test list initialization.
    cat::array from_array{1, 2, 3};
    cat::verify(from_array[0] == 1);
    cat::verify(from_array[1] == 2);
    cat::verify(from_array[2] == 3);
}
