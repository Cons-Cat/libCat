#pragma once

namespace cat {

// Forward declarations.
template <typename abi_type, typename T>
    requires(is_same<typename abi_type::scalar_type, T>)
class alignas(abi_type::alignment.raw) simd;

template <typename abi_type, typename T>
class alignas(abi_type::alignment.raw) simd_mask;

}  // namespace cat

namespace x64 {

// `Sse2abi_type` is a SIMD ABI that can be expected to work on any x86-64 build
// target.
template <typename T>
struct sse42_abi {
    using scalar_type = T;

    // Produce a similar `sse42_abi` for type `U`.
    template <typename U>
    using make_abi_type = sse42_abi<U>;

    sse42_abi() = delete;

    static constexpr cat::idx size = 16u;
    static constexpr cat::uword lanes = size / sizeof(T);
    static constexpr cat::uword alignment = 16u;
};

template <typename T>
using sse42_mask = cat::simd<sse42_abi<T>, T>;

template <typename T>
using sse42_simd_mask = cat::simd_mask<sse42_abi<T>, T>;

enum class string_control : unsigned char {
    // Unsigned 1-byte characters.
    unsigned_byte = 0x00,
    // Unsigned 2-byte characters.
    unsigned_word = 0x01,
    // Signed 1-byte characters.
    signed_byte = 0x02,
    // Signed 2-byte characters.
    signed_word = 0x03,
    // Compare if any characters are equal.
    compare_equal_any = 0x00,
    // Compare ranges.
    compare_ranges = 0x04,
    // Compare if every character is equal.
    compare_equal_each = 0x08,
    // Compare equal ordered.
    compare_equal_ordered = 0x0c,
    // Polarity.
    positive_polarity = 0x00,
    // Negate the results.
    negative_polarity = 0x10,
    masked_positive_polarity = 0x20,
    // Negate the results only before the end of the string.
    masked_negative_polarity = 0x30,
    // Return the least significant bit.
    least_significant = 0x00,
    // Return the most significant bit.
    most_significant = 0x40,
    // Return a bit mask.
    bit_mask = 0x00,
    // Return a byte/word mask.
    unit_mask = 0x40,
};

// TODO: Generalize this.
constexpr auto operator|(string_control flag_1, string_control flag_2)
    -> string_control {
    return static_cast<string_control>(static_cast<unsigned char>(flag_1) |
                                       static_cast<unsigned char>(flag_2));
}

}  // namespace x64
