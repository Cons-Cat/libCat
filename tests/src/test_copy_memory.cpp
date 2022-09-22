#include <cat/array>
#include <cat/memory>
#include <cat/string>

auto main() -> int {
    cat::Array<int4, 200> source_200;
    for (int4 i = 0; i < 200; ++i) {
        source_200[i] = i;
    }

    cat::Array<int4, 200> dest_200;
    cat::copy_memory(&dest_200, &source_200, ssizeof(dest_200));
    for (int4 i = 0; i < 200; ++i) {
        cat::verify(source_200[i] == dest_200[i]);
    }

    cat::Array<int4, 2000> source_2000;
    for (int4 i = 0; i < 2000; ++i) {
        source_2000[i] = i;
    }

    cat::Array<int4, 2000> dest_2000;
    for (int4 i = 0; i < 2000; ++i) {
        dest_2000[i] = 0;
    }

    cat::copy_memory(&source_2000, &dest_2000, ssizeof(dest_2000));
    for (int4 i = 0; i < 2000; ++i) {
        cat::verify(source_2000[i] == dest_2000[i]);
    }
};
