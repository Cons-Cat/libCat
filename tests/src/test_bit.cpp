#include <cat/bit>

#include "../unit_tests.hpp"

TEST(test_bit) {
    // Test clz/ctz.
    static_assert(cat::countl_zero(0X7FFFFFFFFFFFFFFFu) == 1);
    static_assert(cat::countl_zero(0X7FFFFFFFu) == 1);
    static_assert(cat::countl_one(0X7FFFFFFFFFFFFFFFu) == 0);
    static_assert(cat::countl_one(0X7FFFFFFFu) == 0);

    static_assert(cat::countr_zero(0XFFFFFFFFFFFFFFFEu) == 1);
    static_assert(cat::countr_zero(0XFFFFFFFEu) == 1);
    static_assert(cat::countr_one(0XFFFFFFFFFFFFFFFEu) == 0);
    static_assert(cat::countr_one(0XFFFFFFFEu) == 0);

    // Test popcnt.
    static_assert(cat::popcount(0b0101010011u) == 5);
    static_assert(cat::popcount(0b010101001ull) == 4);
    static_assert(cat::popcount(0b010101001_u8) == 4);
    static_assert(cat::popcount(0b010101011_u1) == 5);

    // Test bextr.
    static_assert(cat::extract_bits(uint1::max >> 1, 4u, 4u) == 0b0111u);
    static_assert(cat::extract_bits(uint1::max, 4u, 4u) == 0b1111u);
    static_assert(cat::extract_bits(uint1::max >> 1, 0u, 4u) == 0b1111u);
    static_assert(cat::extract_bits(uint1::max >> 1, 0u, 5u) == 0b11111u);

    static_assert(cat::extract_bits(uint4::max >> 1, 27u, 4u) == 0b1111u);
    static_assert(cat::extract_bits(uint4::max >> 1, 28u, 4u) == 0b0111u);

    static_assert(cat::extract_bits(uint8::max >> 1, 59u, 4u) == 0b1111u);
    static_assert(cat::extract_bits(uint8::max >> 1, 60u, 4u) == 0b0111u);
}
