#include <cat/bit>

#include "../unit_tests.hpp"

TEST(test_bit) {
    using namespace cat::arithmetic_literals;

    // Test clz().
    static_assert(cat::countl_zero(0X7FFFFFFFFFFFFFFFu) == 1);
    static_assert(cat::countl_zero(0X7FFFFFFFu) == 1);
    static_assert(cat::countl_one(0X7FFFFFFFFFFFFFFFu) == 0);
    static_assert(cat::countl_one(0X7FFFFFFFu) == 0);

    // Test ctz().
    static_assert(cat::countr_zero(0XFFFFFFFFFFFFFFFEu) == 1);
    static_assert(cat::countr_zero(0XFFFFFFFEu) == 1);
    static_assert(cat::countr_one(0XFFFFFFFFFFFFFFFEu) == 0);
    static_assert(cat::countr_one(0XFFFFFFFEu) == 0);

    // Test popcnt().
    static_assert(cat::popcount(0b0101010011u) == 5);
    static_assert(cat::popcount(0b010101001ull) == 4);
    static_assert(cat::popcount(0b010101001_u8) == 4);
    static_assert(cat::popcount(0b010101011_u1) == 5);

    // Test bextr().
    static_assert(x64::extract_bits(uint1::max() >> 1, 4u, 4u) == 0b0111u);
    static_assert(x64::extract_bits(uint1::max(), 4u, 4u) == 0b1111u);
    static_assert(x64::extract_bits(uint1::max() >> 1, 0u, 4u) == 0b1111u);
    static_assert(x64::extract_bits(uint1::max() >> 1, 0u, 5u) == 0b11111u);

    static_assert(x64::extract_bits(uint4::max() >> 1, 27u, 4u) == 0b1111u);
    static_assert(x64::extract_bits(uint4::max() >> 1, 28u, 4u) == 0b0111u);

    static_assert(x64::extract_bits(uint8::max() >> 1, 59u, 4u) == 0b1111u);
    static_assert(x64::extract_bits(uint8::max() >> 1, 60u, 4u) == 0b0111u);

    // Test pext().
    static_assert(x64::extract_bits_mask(uint8::max(), 0b1ull) == 0b1ull);
    static_assert(x64::extract_bits_mask(uint4::max(), 0b1u) == 0b1u);
    static_assert(x64::extract_bits_mask(uint2::max(), 0b1_u2) == 0b1_u2);

    // Test bzhi().
    // TODO: These only test it compiles. Test that it works correctly.
    cat::assert(x64::zero_high_bits_at(8_u4, 8u) != 0u);
    cat::assert(x64::zero_high_bits_at(8_u8, 8u) != 0u);

    // Test `bit_value`.
    cat::bit_value bit1 = false;
    bit1 = true;
    cat::verify(~bit1 == false);
    cat::verify(~~bit1 == true);
    cat::verify(false == ~bit1);
    // NOLINTNEXTLINE
    cat::verify(bit1 == bit1);

    // Test `bit_reference`
    unsigned char number = 0u;
    cat::bit_reference bit2 =
        cat::bit_reference<unsigned char>::from_mask(number, 1u);
    bit2 = true;
    cat::verify(bit1.is_set());
    cat::verify(bit1 == true);
    cat::verify(bit2.is_set());
    cat::verify(bit2);

    cat::bit_reference bit3 = bit1;
    bit3.set();
    cat::verify(bit3 == true);
    cat::verify(number != 0u);

    cat::bit_reference bit4 =
        cat::bit_reference<unsigned char>::from_offset(number, 0);
    cat::bit_reference bit5 =
        cat::bit_reference<unsigned char>::from_offset(number, 5);
    cat::verify(bit4.is_set());
    cat::verify(!bit5.is_set());
    number = 0;
    cat::verify(!bit4.is_set());
    cat::verify(!bit5.is_set());
    number = 1 << 5;
    cat::verify(!bit4.is_set());
    cat::verify(bit5.is_set());

    // Assign a `bit_reference` to a `bit_value`.
    bit1 = bit2;
    bit1 = bit3;

    // Test `bit_iterator`.
    cat::array<uint4, 4> array{0, cat::uint4_max, 0, 0};
    cat::bit_iterator it = array.begin();
    cat::verify(*it == false);
    cat::verify(*(it + 31) == false);
    cat::verify(*(it + 32) == true);
    it += 33;
    cat::verify(*it == true);

    for (int4 i = 0; i < 30; ++i) {
        ++it;
        cat::verify(*it == true);
    }

    it++;
    cat::verify(*it == false);
}
