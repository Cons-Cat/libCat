// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// wyhash is Wang Yi's fast non-cryptographic hash. `cat::wyhash()` matches the
// reference `wyhash()` and `wyhash64()` with the default secret.
//    https://github.com/wangyi-fudan/wyhash
//
// It hashes a span of bytes, a SIMD value, or an integer far more cheaply than
// hashing its bytes. Any of them may be given a seed.
//
//    uint8 bytes_hash = cat::wyhash(bytes);
//    uint8 integer_hash = cat::wyhash(42, seed);
//
// Multiple hashes may be computed in parallel with `cat::simd`.
//
//    cat::fixed_size_simd<uint4, 4u> value{1u, 2u, 3u, 4u};
//    auto simd_hash = cat::wyhash(value, seed);
//
// This code is tested in `tests/src/test_hash.cpp` against the reference
// implementation.

#include <cat/arithmetic>
#include <cat/array>
#include <cat/simd>
#include <cat/span>

namespace cat {

namespace detail {

inline constexpr array<uint8, 4u> wyhash_secret = {
   0x2d358dcc'aa6c78a5ull, 0x8bb84b93'962eacc9ull, 0x4b33a62e'd433d4a3ull,
   0x4d5a2da5'1de1aa47ull
};

// Replace `a` and `b` with the low and high halves of their 128-bit product.
// TODO: Use a libCat 128-bit integer when one exists.
constexpr void
wyhash_multiply(uint8& a, uint8& b) {
   unsigned __int128 const product =
      static_cast<unsigned __int128>(a.raw) * b.raw;
   a = static_cast<uint8::raw_type>(product);
   b = static_cast<uint8::raw_type>(product >> 64u);
}

[[nodiscard]]
constexpr auto
wyhash_mix(uint8 a, uint8 b) -> uint8 {
   wyhash_multiply(a, b);
   return a ^ b;
}

// Read `count` bytes starting at `offset` as a little-endian integer.
[[nodiscard]]
constexpr auto
wyhash_read(span<byte const> bytes, idx offset, idx count) -> uint8 {
   uint8 value = 0u;
   for (idx i = 0u; i < count; ++i) {
      value |= uint8(bytes[offset + i].value) << (i * 8u);
   }
   return value;
}

struct wyhash_impl {
   // Hash a string of bytes.
   [[nodiscard]]
   static constexpr auto
   operator()(span<byte const> bytes, uint8 seed = 0u) -> uint8 {
      using detail::wyhash_mix;
      using detail::wyhash_read;
      auto const& secret = detail::wyhash_secret;

      // Every subtraction of `length`, `step`, and `remaining` below is guarded
      // against going negative, so it is converted back to `idx`.
      idx const length = bytes.size();
      uint8 state = seed ^ wyhash_mix(seed ^ secret[0u], secret[1u]);
      uint8 a = 0u;
      uint8 b = 0u;

      if (length <= 16u) {
         if (length >= 4u) {
            // Two overlapping pairs of 4-byte reads cover 4 to 16 bytes.
            idx const step = (length / 8u) * 4u;
            a = (wyhash_read(bytes, 0u, 4u) << 32u)
                | wyhash_read(bytes, step, 4u);
            b = (wyhash_read(bytes, idx(length - 4u), 4u) << 32u)
                | wyhash_read(bytes, idx(length - 4u - step), 4u);
         } else if (length > 0u) {
            a = (wyhash_read(bytes, 0u, 1u) << 16u)
                | (wyhash_read(bytes, length / 2u, 1u) << 8u)
                | wyhash_read(bytes, idx(length - 1u), 1u);
         }
      } else {
         idx offset = 0u;
         idx remaining = length;
         if (remaining >= 48u) {
            uint8 state_1 = state;
            uint8 state_2 = state;
            do {
               state = wyhash_mix(
                  wyhash_read(bytes, offset, 8u) ^ secret[1u],
                  wyhash_read(bytes, offset + 8u, 8u) ^ state
               );
               state_1 = wyhash_mix(
                  wyhash_read(bytes, offset + 16u, 8u) ^ secret[2u],
                  wyhash_read(bytes, offset + 24u, 8u) ^ state_1
               );
               state_2 = wyhash_mix(
                  wyhash_read(bytes, offset + 32u, 8u) ^ secret[3u],
                  wyhash_read(bytes, offset + 40u, 8u) ^ state_2
               );
               offset += 48u;
               remaining = idx(remaining - 48u);
            } while (remaining >= 48u);
            state ^= state_1 ^ state_2;
         }
         while (remaining > 16u) {
            state = wyhash_mix(
               wyhash_read(bytes, offset, 8u) ^ secret[1u],
               wyhash_read(bytes, offset + 8u, 8u) ^ state
            );
            offset += 16u;
            remaining = idx(remaining - 16u);
         }
         // The last 16 bytes may overlap bytes that were already mixed.
         idx const end = offset + remaining;
         a = wyhash_read(bytes, idx(end - 16u), 8u);
         b = wyhash_read(bytes, idx(end - 8u), 8u);
      }

      a ^= secret[1u];
      b ^= state;
      detail::wyhash_multiply(a, b);
      return wyhash_mix(a ^ secret[0u] ^ uint8(length), b ^ secret[1u]);
   }

   // Hash an integer. This is the reference `wyhash64()`, which is much cheaper
   // than hashing the integer's bytes.
   template <is_integral T>
   [[nodiscard]]
   static constexpr auto
   operator()(T value, uint8 seed = 0u) -> uint8 {
      auto const& secret = detail::wyhash_secret;
      uint8 a = uint8(make_raw_arithmetic(value)) ^ secret[0u];
      uint8 b = seed ^ secret[1u];
      detail::wyhash_multiply(a, b);
      return detail::wyhash_mix(a ^ secret[0u], b ^ secret[1u]);
   }

   template <is_simd Simd>
   [[nodiscard]]
   static constexpr auto
   operator()(Simd const& value, uint8 seed = 0u) {
      using result_abi = cat::simd_abi::deduce<
         uint8, Simd::abi_type::lanes, typename Simd::abi_type>;
      simd<uint8, result_abi> result{};
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         auto const lane_value = value[lane];
         result.set_lane(lane, wyhash_impl::operator()(lane_value, seed));
      }
      return result;
   }
};

}  // namespace detail

// `cat::wyhash` is a function object, so its type can be a container's
// `Hash` parameter.
inline constexpr detail::wyhash_impl wyhash{};

}  // namespace cat
