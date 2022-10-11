#include <cat/simd>

#include "../unit_tests.hpp"

// TODO: Test more SIMD functions.
TEST(test_simd) {
    // Test that vector arithmetic does not segfault.
    int4x4 vec1 = {0, 1, 2, 3};
    int4x4 vec2{0, 1, 2, 3};
    vec1 += vec2;
    _ = vec1 + vec2;

    // TODO: Test correctness of vector operations.
}
