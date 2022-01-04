// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd>

// TODO: Constrain parameter with a vector concept.
// TODO: This code can be simplified a lot.
/* Non-temporally copy a vector into some address. */
template <typename T>
void simd::stream_in(void* p_destination, T const* source) {
    // TODO: Make an integral-vector concept to simplify this.
    // Streaming 4-byte floats.
    if constexpr (meta::is_same_v<T, f4x4>) {
        __builtin_ia32_movntps(p_destination, source);
    } else if constexpr (meta::is_same_v<T, f4x8>) {
        __builtin_ia32_movntps256(p_destination, source);
    }
    // Streaming 8-byte floats.
    if constexpr (meta::is_same_v<T, f8x2>) {
        __builtin_ia32_movntpd(p_destination, source);
    } else if constexpr (meta::is_same_v<T, f8x4>) {
        __builtin_ia32_movntpd256(p_destination, source);
    }
    // Streaming 1-byte ints.
    else if constexpr (meta::is_same_v<T, u1x4> || meta::is_same_v<T, i1x4>) {
        __builtin_ia32_movnti(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x8> || meta::is_same_v<T, i1x8>) {
        __builtin_ia32_movntq(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x16> ||
                         meta::is_same_v<T, i1x16>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x32> ||
                         meta::is_same_v<T, i1x32>) {
        __builtin_ia32_movntq256(p_destination, source);
    }
    // Streaming 2-byte ints.
    else if constexpr (meta::is_same_v<T, u2x2> || meta::is_same_v<T, i2x2>) {
        __builtin_ia32_movnti(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u2x4> || meta::is_same_v<T, i2x4>) {
        __builtin_ia32_movntq(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u2x8> || meta::is_same_v<T, i2x8>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u2x16> ||
                         meta::is_same_v<T, i2x16>) {
        __builtin_ia32_movntq256(p_destination, source);
    }
    // Streaming 4-byte ints.
    else if constexpr (meta::is_same_v<T, u4x2> || meta::is_same_v<T, i4x2>) {
        __builtin_ia32_movntq(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u4x4> || meta::is_same_v<T, i4x4>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u4x8> || meta::is_same_v<T, i4x8>) {
        __builtin_ia32_movntdq256(p_destination, source);
    }
    // Streaming 8-byte ints.
    else if constexpr (meta::is_same_v<T, u1x2> || meta::is_same_v<T, i1x2>) {
        __builtin_ia32_movntq128(p_destination, source);
    } else if constexpr (meta::is_same_v<T, u1x4> || meta::is_same_v<T, i1x4>) {
        __builtin_ia32_movntdq256(p_destination, source);
    }
}
