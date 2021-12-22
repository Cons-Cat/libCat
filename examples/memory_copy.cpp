#include <buffer.h>
#include <string.h>

void meow() {
    Buffer<i4, 200000> source_2000;
    Buffer<i4, 200000> dest_2000;
    simd::copy_memory(&source_2000, &dest_2000, sizeof(dest_2000));
    // Prevent these from being optimized out.
    asm volatile("" ::"m"(source_2000));
    asm volatile("" ::"m"(dest_2000));
    exit(0);
}
