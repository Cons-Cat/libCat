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
    bits7_2 = cat::Bitset<7>::from(0b1111111_u1);

    cat::Bitset<128> bits128 = cat::Bitset<128>::from(0x0_u8, 0x0_u8);
}
