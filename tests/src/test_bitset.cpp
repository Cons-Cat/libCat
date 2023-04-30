#include <cat/bitset>

#include "../unit_tests.hpp"

TEST(test_bitset) {
    using namespace cat::arithmetic_literals;

    constexpr cat::bitset<7u> bits7{};
    constexpr cat::bitset<8u> bits8{};
    constexpr cat::bitset<16u> bits16{};
    constexpr cat::bitset<17u> bits17{};
    constexpr cat::bitset<32u> bits32{};
    constexpr cat::bitset<64u> bits64{};
    constexpr cat::bitset<65u> bits65{};
    constexpr cat::bitset<129u> bits129{};

    static_assert(bits7.storage_array_size == 1);
    static_assert(bits8.storage_array_size == 1);
    static_assert(bits16.storage_array_size == 1);
    static_assert(bits17.storage_array_size == 1);
    static_assert(bits32.storage_array_size == 1);
    static_assert(bits64.storage_array_size == 1);
    static_assert(bits65.storage_array_size == 2);
    static_assert(bits129.storage_array_size == 3);

    static_assert(bits7.storage_element_size == 1);
    static_assert(bits8.storage_element_size == 1);
    static_assert(bits16.storage_element_size == 2);
    static_assert(bits17.storage_element_size == 4);
    static_assert(bits32.storage_element_size == 4);
    static_assert(bits64.storage_element_size == 8);
    static_assert(bits65.storage_element_size == 8);
    static_assert(bits129.storage_element_size == 8);

    cat::bitset<7u> bits7_2 = bits7;
    bits7_2 = cat::bitset<7u>::from(0x0_u1);
    cat::verify(!bits7_2.all_of());
    cat::verify(bits7_2.none_of());
    cat::verify(!bits7_2.any_of());

    bits7_2 = cat::bitset<7u>::from(0b01111111_u1);
    cat::verify(bits7_2.all_of());
    cat::verify(!bits7_2.none_of());
    cat::verify(bits7_2.any_of());

    bits7_2 = cat::bitset<7u>::from(0b01000000_u1);
    cat::verify(!bits7_2.all_of());
    cat::verify(!bits7_2.none_of());
    cat::verify(bits7_2.any_of());

    bits7_2 = cat::bitset<7u>::from(0b10000000_u1);
    cat::verify(!bits7_2.all_of());
    cat::verify(!bits7_2.none_of());
    cat::verify(bits7_2.any_of());

    cat::bitset<127u> bits127 = cat::bitset<127u>::from(0x0_u8, 0x0_u8);
    cat::verify(!bits127.all_of());
    cat::verify(bits127.none_of());
    cat::verify(!bits127.any_of());

    // The 128th bit is off, all_of others are on.
    bits127 =
        cat::bitset<127u>::from(cat::uint8_max >> 1u, 0xFFFFFFFF'FFFFFFFF_u8);
    cat::verify(bits127.leading_bytes_bits == 63u);
    cat::verify(bits127.all_of());
    cat::verify(!bits127.none_of());
    cat::verify(bits127.any_of());
    cat::verify(bits127.countl_zero() == 0u);
    cat::verify(bits127.countr_zero() == 0u);

    // The 128th bit is off, all_of others are on.
    bits127 = cat::bitset<127u>::from(cat::uint8_max >> 2u,
                                      0xFFFFFFFF'FFFFFFFF_u8 << 1u);
    cat::verify(bits127.countl_zero() == 1u);
    cat::verify(bits127.countr_zero() == 1u);

    bits127 = cat::bitset<127u>::from(0_u8, cat::uint8_max >> 1u);
    cat::verify(bits127.countl_zero() == 64u);
    cat::verify(bits127.countr_zero() == 0u);

    // Test const subscripting.
    constexpr cat::bitset<15u> bits15 =
        cat::bitset<15u>::from(0b0101010101010100_u2);
    static_assert(!bits15[0u]);
    static_assert(bits15[1u]);
    static_assert(!bits15[2u]);

    constexpr cat::bitset<15u> bits15_2 =
        cat::bitset<15u>::from(0b1010101010101010_u2);
    static_assert(bits15_2[0u]);
    static_assert(!bits15_2[1u]);
    static_assert(bits15_2[2u]);

    // Test 16 byte bitset's subscript.
    constexpr cat::bitset<128u> bits128_2 =
        cat::bitset<128u>::from(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);
    // 11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111011
    static_assert(bits128_2[0u]);
    static_assert(bits128_2[1u]);
    static_assert(!bits128_2[2u]);
    static_assert(bits128_2[125u]);
    static_assert(bits128_2[126u]);
    static_assert(bits128_2[127u]);

    // Test 16 byte bitset's subscript with bit offset.
    constexpr cat::bitset<127u> bits127_2 =
        cat::bitset<127u>::from(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);
    // 11111111'11111111'11111111'11111111'11111111'11111111'11111111'1111101
    static_assert(bits127_2[0u]);
    static_assert(!bits127_2[1u]);
    static_assert(bits127_2[2u]);
    static_assert(bits127_2[125u]);
    static_assert(bits127_2[126u]);

    // Test mutable subscript.
    bits127 = cat::bitset<127u>::from(cat::uint8_max >> 2u, 0b00000100_u8);

    cat::verify(!bits127[0u]);
    cat::verify(bits127[1u]);

    bits127[1u] = false;
    cat::verify(!bits127[1u]);
    bits127[1u] = true;
    cat::verify(bits127[1u]);

    bits127[0u] = true;
    cat::verify(bits127[0u]);
    bits127[0u] = false;
    cat::verify(!bits127[0u]);

    // Test this on the second element of uint8 array.
    // `bitset` is zero-indexed, so the largest addressable bit is 126.
    // The left-most two bits are unset, so 126 and 125 should be unset, but 124
    // should be set.
    cat::verify(!bits127[125u]);
    cat::verify(bits127[124u]);
    bits127[124u] = false;
    cat::verify(!bits127[124u]);
    bits127[124u] = true;
    cat::verify(bits127[124u]);

    // Test const `.at()`.
    _ = bits127_2.at(0u).verify();
    cat::verify(!bits127_2.at(128u).has_value());

    // Test mutable `.at()`.
    bits127.at(0u).verify() = true;
    cat::verify(bits127.at(0u).has_value());
    cat::verify(!bits127.at(128u).has_value());

    // Test const iterator.
    for ([[maybe_unused]] auto bit : bits127_2) {
    }

    // Test mutable iterator.
    for (cat::bit_reference bit : bits127) {
        bit = false;
    }
    for (cat::bit_reference bit : bits127) {
        cat::verify(bit == false);
    }

    for (cat::bit_reference bit : bits127) {
        bit = true;
    }
    for (cat::bit_reference bit : bits127) {
        cat::verify(bit == true);
    }
}
