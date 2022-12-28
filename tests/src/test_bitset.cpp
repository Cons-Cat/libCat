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
    bits7_2 = cat::Bitset<7>::from(0x0u1);
    cat::verify(!bits7_2.all_of());
    cat::verify(bits7_2.none_of());
    cat::verify(!bits7_2.any_of());

    bits7_2 = cat::Bitset<7>::from(0b01111111u1);
    cat::verify(bits7_2.all_of());
    cat::verify(!bits7_2.none_of());
    cat::verify(bits7_2.any_of());

    bits7_2 = cat::Bitset<7>::from(0b01000000u1);
    cat::verify(!bits7_2.all_of());
    cat::verify(!bits7_2.none_of());
    cat::verify(bits7_2.any_of());

    bits7_2 = cat::Bitset<7>::from(0b10000000u1);
    cat::verify(!bits7_2.all_of());
    cat::verify(!bits7_2.none_of());
    cat::verify(bits7_2.any_of());

    cat::Bitset<127> bits127 = cat::Bitset<127>::from(0x0u8, 0x0u8);
    cat::verify(!bits127.all_of());
    cat::verify(bits127.none_of());
    cat::verify(!bits127.any_of());

    // The 128th bit is off, all_of others are on.
    bits127 =
        cat::Bitset<127>::from(cat::uint8_max >> 1u, 0xFFFFFFFF'FFFFFFFFu8);
    cat::verify(bits127.leading_bytes_bits == 63);
    cat::verify(bits127.all_of());
    cat::verify(!bits127.none_of());
    cat::verify(bits127.any_of());
    cat::verify(bits127.countl_zero() == 0);
    cat::verify(bits127.countr_zero() == 0);

    // The 128th bit is off, all_of others are on.
    bits127 = cat::Bitset<127>::from(cat::uint8_max >> 2u,
                                     0xFFFFFFFF'FFFFFFFFu8 << 1u);
    cat::verify(bits127.countl_zero() == 1);
    cat::verify(bits127.countr_zero() == 1);

    bits127 = cat::Bitset<127>::from(0u8, cat::uint8_max >> 1u);
    cat::verify(bits127.countl_zero() == 64);
    cat::verify(bits127.countr_zero() == 0);

    // Test const subscripting.
    constexpr cat::Bitset<15> bits15 =
        cat::Bitset<15>::from(0b010101010101010u2);
    static_assert(!bits15[0]);
    static_assert(bits15[1]);
    static_assert(!bits15[2]);

    constexpr cat::Bitset<15> bits15_2 =
        cat::Bitset<15>::from(0b101010101010101u2);
    static_assert(bits15_2[0]);
    static_assert(!bits15_2[1]);
    static_assert(bits15_2[2]);

    // Test 16 byte bitset's subscript.
    constexpr cat::Bitset<128> bits128_2 =
        cat::Bitset<128>::from(0xFFFFFFFF'FFFFFFFFu8, 0xFFFFFFFF'FFFFFFFBu8);
    static_assert(bits128_2[0]);
    static_assert(bits128_2[1]);
    static_assert(bits128_2[2]);
    static_assert(!bits128_2[125]);
    static_assert(bits128_2[126]);
    static_assert(bits128_2[127]);

    // Test 16 byte bitset's subscript with bit offset.
    constexpr cat::Bitset<127> bits127_2 =
        cat::Bitset<127>::from(0xFFFFFFFF'FFFFFFFFu8, 0xFFFFFFFF'FFFFFFFBu8);
    static_assert(bits127_2[0]);
    static_assert(bits127_2[1]);
    static_assert(bits127_2[2]);
    static_assert(bits127_2[125]);
    static_assert(!bits127_2[126]);

    // Test mutable subscript.
    bits127 = cat::Bitset<127>::from(cat::uint8_max >> 2u,
                                     0xFFFFFFFF'FFFFFFFFu8 << 1u);
    cat::verify(!bits127[0]);
    cat::verify(bits127[1]);

    bits127[1] = false;
    cat::verify(!bits127[1]);
    bits127[1] = true;
    cat::verify(bits127[1]);

    bits127[0] = true;
    cat::verify(bits127[0]);
    bits127[0] = false;
    cat::verify(!bits127[0]);

    // `const` subscript returns bool.
    static_assert(cat::is_same<decltype(bits127_2[0]), bool>);
    // Non-`const` subscript returns a `BitReference`.
    static_assert(!cat::is_same<decltype(bits127[0]), bool>);

    // Test this on the second element of uint8 array.
    cat::verify(bits127[126]);
    bits127[126] = false;
    cat::verify(!bits127[126]);
    bits127[126] = true;
    cat::verify(bits127[126]);

    // Test const `.at()`.
    _ = bits127_2.at(0).verify();
    cat::verify(!bits127_2.at(128).has_value());

    // Test mutable `.at()`.
    bits127.at(0).verify() = true;
    cat::verify(bits127.at(0).has_value());
    cat::verify(!bits127.at(128).has_value());

    // Test const iterator.
    // for ([[maybe_unused]] bool bit : bits127_2) {
    // }

    // // Test mutable iterator.
    // for (cat::BitReference bit : bits127) {
    //     bit = false;
    // }

    // for (cat::BitReference bit : bits127) {
    //     cat::verify(bit == false);
    // }
}
