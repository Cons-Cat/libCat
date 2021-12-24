#include <buffer.h>

void meow() {
    // Initializing a buffer:
    Buffer<i4, 5> buffer_1 = {0, 1, 2, 3, 4};
    // Assigning a buffer:
    buffer_1 = {5, 6, 7, 8, 9};
    // Default initializing a buffer:
    Buffer<i4, 1> buffer_2;
    // Move assigning a buffer:
    Buffer<i4, 1> buffer_3;
    buffer_2 = meta::move(buffer_3);
    // Move constructing a buffer:
    _ = meta::move(buffer_1);

    // Repeat those tests in a constexpr context.
    auto constant = []() constexpr {
        Buffer<i4, 1> const_buffer_1;
        Buffer<i4, 1> const_buffer_2 = {1};
        _ = meta::move(const_buffer_1);
        _ = meta::move(const_buffer_2);
    };
    meta::constant_evaluate(constant);
    exit(0);
};
