#include <buffer.h>

#include "start.h"

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
    Buffer<i4, 5> buffer_4(meta::move(buffer_1));
    dont_optimize_out(buffer_4);

    // Repeat those tests in a constexpr context.
    constexpr auto constant = []() {
        Buffer<i4, 1> const_buffer_1;
        Buffer<i4, 1> const_buffer_2 = {1};
        Buffer<i4, 1> const_buffer_3 = meta::move(const_buffer_1);
        Buffer<i4, 1> const_buffer_4(meta::move(const_buffer_2));
        dont_optimize_out(const_buffer_3);
        dont_optimize_out(const_buffer_4);
    };
    constant();
};
