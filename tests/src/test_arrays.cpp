#include <cat/array>
#include <cat/math>
#include <cat/runtime>
#include <cat/string>
#include <cat/utility>

#include "../unit_tests.hpp"

TEST(test_arrays) {
    // Initializing a array:
    cat::array<int4, 5> array_1 = {0, 1, 2, 3, 4};
    // Assigning a array:
    array_1 = {5, 6, 7, 8, 9};
    // Default initializing a array:
    cat::array<int4, 1> array_2;
    // Move assigning a array:
    cat::array<int4, 1> array_3 = {0};
    array_2 = cat::move(array_3);  // NOLINT
    // Move constructing a array:
    _ = cat::move(array_1);  // NOLINT

    // `const` array.
    cat::array<int4, 3> const array_const = {0, 1, 2};
    [[maybe_unused]] int4 const_val = array_const.at(1).or_exit();

    // Repeat those tests in a constexpr context.
    auto constant_test = []() constexpr {
        cat::array<int4, 1> const_array_1;
        cat::array<int4, 1> const_array_2 = {1};
        // NOLINTNEXTLINE Just be explicit about the move here.
        _ = cat::move(const_array_1);
        // NOLINTNEXTLINE Just be explicit about the move here.
        _ = cat::move(const_array_2);
    };
    cat::constant_evaluate(constant_test);

    // Test that the array is iterable.
    iword count = 0;
    for (int4& a : array_1) {
        cat::verify(a == array_1[count]);
        ++count;
    }

    for (int4 const& a : cat::as_reverse(array_1)) {
        --count;
        cat::verify(a == array_1[count]);
    }

    cat::verify(array_1.front() == 5);
    cat::verify(array_1.back() == 9);

    count = 0;
    for (int4 const& a : cat::as_const(array_1)) {
        cat::verify(a == array_1[count]);
        ++count;
    }
    _ = array_1.cbegin();

    for (int4 const& a : cat::as_const_reverse(array_1)) {
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
    static_assert(implicit_array_1.size() == 5);
    static_assert(implicit_array_1.capacity() == 5);
    _ = implicit_array_1.capacity();
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
    [[maybe_unused]] cat::span span = array_1.first(1);
    _ = array_1.subspan(0, 2);
    _ = array_1.last(2);

    [[maybe_unused]] cat::span const span_const = array_1.first(1);
    _ = array_const.subspan(0, 2);
    _ = array_const.last(2);

    // Test array copy-assignment.
    cat::array base_array = {0, 0, 0, 0};
    cat::array copy_array = {1, 2, 3, 4};
    cat::array copy_converting_array = {int2{1}, 2, 3, 4};
    cat::array move_array = {5, 6, 7, 8};
    cat::array move_converting_array = {int2{5}, 6, 7, 8};
    base_array = copy_array;
    base_array = copy_converting_array;
    base_array = move(move_array);             // NOLINT
    base_array = move(move_converting_array);  // NOLINT

    // Test array fill.
    cat::array filled_array = cat::array<int4, 8>::filled(6);
    for (iword i = 0; i < 8; ++i) {
        cat::verify(filled_array[i] == 6);
    }
    filled_array.fill(9);
    for (iword i = 0; i < 8; ++i) {
        cat::verify(filled_array[i] == 9);
    }

    // Test from factory.
    cat::array from_array = cat::array<int4, 3>::from(1, 2, 3);
    cat::verify(from_array[0] == 1);
    cat::verify(from_array[1] == 2);
    cat::verify(from_array[2] == 3);
}
