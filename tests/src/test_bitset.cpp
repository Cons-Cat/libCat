#include <cat/bitset>

#include "../unit_tests.hpp"

TEST(test_bitset) {
    constexpr cat::Bitset<7> bits7{};
    constexpr cat::Bitset<8> bits8{};
    constexpr cat::Bitset<16> bits16{};
    constexpr cat::Bitset<17> bits17{};
    constexpr cat::Bitset<32> bits32{};
    constexpr cat::Bitset<64> bits64{};
    constexpr cat::Bitset<65> bits65{};
    constexpr cat::Bitset<129> bits129{};

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

    cat::Bitset<7> bits7_2 = bits7;
    bits7_2 = cat::Bitset<7>::from(0x0_u1);
    cat::verify(!bits7_2.all());
    cat::verify(bits7_2.none());
    cat::verify(!bits7_2.any());

    bits7_2 = cat::Bitset<7>::from(0b01111111_u1);
    cat::verify(bits7_2.all());
    cat::verify(!bits7_2.none());
    cat::verify(bits7_2.any());

    bits7_2 = cat::Bitset<7>::from(0b01000000_u1);
    cat::verify(!bits7_2.all());
    cat::verify(!bits7_2.none());
    cat::verify(bits7_2.any());

    bits7_2 = cat::Bitset<7>::from(0b10000000_u1);
    cat::verify(!bits7_2.all());
    cat::verify(!bits7_2.none());
    cat::verify(bits7_2.any());

    cat::Bitset<127> bits127 = cat::Bitset<127>::from(0x0_u8, 0x0_u8);
    cat::verify(!bits127.all());
    cat::verify(bits127.none());
    cat::verify(!bits127.any());

    // The 128th bit is off, all others are on.
    bits127 =
        cat::Bitset<127>::from(cat::uint8_max >> 1u, 0xFFFFFFFFFFFFFFFF_u8);
    cat::verify(bits127.leading_bytes_bits == 63);
    cat::verify(bits127.all());
    cat::verify(!bits127.none());
    cat::verify(bits127.any());
    cat::verify(bits127.countl_zero() == 0);
    cat::verify(bits127.countr_zero() == 0);

    // The 128th bit is off, all others are on.
    bits127 = cat::Bitset<127>::from(cat::uint8_max >> 2u,
                                     0xFFFFFFFFFFFFFFFF_u8 << 1u);
    cat::verify(bits127.countl_zero() == 1);
    cat::verify(bits127.countr_zero() == 1);

    bits127 = cat::Bitset<127>::from(0_u8, cat::uint8_max >> 1u);
    cat::verify(bits127.countl_zero() == 64);
    cat::verify(bits127.countr_zero() == 0);
}
