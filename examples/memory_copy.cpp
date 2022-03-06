#include <buffer>
#include <string>

void meow() {
    Buffer<int4, 200000> source_2000;
    Buffer<int4, 200000> dest_2000;
    simd::copy_memory(&source_2000, &dest_2000, sizeof(dest_2000));
    exit(0);
}
