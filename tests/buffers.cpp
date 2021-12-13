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
    // TODO: This does not work.
    // Move constructing a buffer:
    // Buffer<i32, 5> buffer_4(move(buffer_1));
};
