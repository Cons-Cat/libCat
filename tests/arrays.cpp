#include <array>

void meow() {
    // Initializing a array:
    Array<int4, 5> array_1 = {0, 1, 2, 3, 4};
    // Assigning a array:
    array_1 = {5, 6, 7, 8, 9};
    // Default initializing a array:
    Array<int4, 1> array_2;
    // Move assigning a array:
    Array<int4, 1> array_3;
    array_2 = cat::move(array_3);  // NOLINT
    // Move constructing a array:
    _ = cat::move(array_1);  // NOLINT

    // Repeat those tests in a constexpr context.
    auto constant = []() constexpr {
        Array<int4, 1> const_array_1;
        Array<int4, 1> const_array_2 = {1};
        // NOLINTNEXTLINE Just be explicit about the move here.
        _ = cat::move(const_array_1);
        // NOLINTNEXTLINE Just be explicit about the move here.
        _ = cat::move(const_array_2);
    };
    _ = meta::constant_evaluate(constant);

    // Test iterable.
    ssize count = 0;
    for (int4 const& a : array_1) {
        Result(a == array_1[count]).or_panic();
        count++;
    }

    for (int4 const& a : cat::Reverse(array_1)) {
        count--;
        Result(a == array_1[count]).or_panic();
    }

    Result(array_1.front() == 5).or_panic();
    Result(array_1.back() == 9).or_panic();

    // Index in and out of bounds.
    Result(array_1.at(0).value() == 5).or_panic();
    Result(!array_1.at(6).has_value()).or_panic();

    cat::exit();
};
