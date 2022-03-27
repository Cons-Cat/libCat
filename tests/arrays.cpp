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
    array_2 = meta::move(array_3);
    // Move constructing a array:
    _ = meta::move(array_1);

    // Repeat those tests in a constexpr context.
    auto constant = []() constexpr {
        Array<int4, 1> const_array_1;
        Array<int4, 1> const_array_2 = {1};
        _ = meta::move(const_array_1);
        _ = meta::move(const_array_2);
    };
    meta::constant_evaluate(constant);

    cat::exit();
};
