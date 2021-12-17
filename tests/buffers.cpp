#include <buffer.h>

void meow() {
    // Initializing a buffer:
    Buffer<i32, 5> buffer_1 = {0, 1, 2, 3, 4};
    // Assigning a buffer:
    buffer_1 = {5, 6, 7, 8, 9};
    // Default initializing a buffer:
    Buffer<i32, 1> buffer_2;
    // Move assigning a buffer:
    Buffer<i32, 1> buffer_3;
    buffer_2 = move(buffer_3);
    // Move constructing a buffer:
    Buffer<i32, 5> buffer_4(move(buffer_1));

    // Repeat those tests in a constexpr context.
    constexpr auto constant = []() {
        Buffer<i32, 1> const_buffer_1;
        Buffer<i32, 1> const_buffer_2 = {1};
        Buffer<i32, 1> const_buffer_3 = move(const_buffer_1);
        Buffer<i32, 1> const_buffer_4(move(const_buffer_2));
    };
    constant();
};
