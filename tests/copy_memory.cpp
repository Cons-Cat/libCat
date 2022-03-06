#include <buffer>
#include <string>

void meow() {
    Buffer<int4, 200> source_200;
    for (int4 i = 0; i < 200; i++) {
        source_200[i] = i;
    }

    Buffer<int4, 200> dest_200;
    copy_memory(&dest_200, &source_200, sizeof(dest_200));
    for (int4 i = 0; i < 200; i++) {
        Result(source_200[i] == dest_200[i]).or_panic();
    }

    Buffer<int4, 2000> source_2000;
    for (int4 i = 0; i < 2000; i++) {
        source_2000[i] = i;
    }

    Buffer<int4, 2000> dest_2000;
    for (int4 i = 0; i < 2000; i++) {
        dest_2000[i] = 0;
    }
    simd::copy_memory(&source_2000, &dest_2000, sizeof(dest_2000));
    for (int4 i = 0; i < 2000; i++) {
        Result(source_2000[i] == dest_2000[i]).or_panic();
    }
    exit(0);
};
