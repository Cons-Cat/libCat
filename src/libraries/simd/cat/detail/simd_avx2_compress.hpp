// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/simd>

#include "cat/detail/simd_abi_hooks.hpp"
#include "cat/detail/simd_avx2_mask_ops.hpp"

// AVX2 stream-compaction overrides for `simd_abi::compress`. Pre-AVX-512 there
// is no `vpcompress*` instruction. Two recipes cover the four supported lane
// widths.
//
// 4- and 8-byte lanes use a per-mask permutation lookup table indexed by the
// `movmsk` lane-bit pattern. A single `vpermd`/`vpermps` rearranges the
// vector, and a `popcount(mask) > {0,1,..}` compare ANDs the inactive tail
// to zero. The 8-byte path drives `vpermd` with index pairs because AVX2 has
// no variable-index `vpermq`/`vpermpd`.
//
// 2- and 1-byte lanes split the 32-byte vector into four 8-byte chunks and
// compress each chunk with BMI2 `pdep`/`pext`. Each chunk's compressed
// output streams into a stack buffer at a `popcount`-driven offset, and the
// buffer reloads as the result vector. `pext` zero-pads above the active
// bits, so the trailing slots come out zero without an explicit tail-mask
// step. BMI2 is available on every AVX2-capable µarch.
//
// Full 16x2-byte and 32x1-byte permutation lookup tables would be 2 MiB and
// 128 GiB respectively, so the lookup-table recipe is not viable for those
// widths and AVX-512 VBMI2 (`vpcompressb`/`vpcompressw`) is the practical
// home for any narrower lane work that wants a single instruction.

namespace cat::detail::simd_abi {

namespace compress_detail {

// One 8-lane permutation index vector. `alignas(32)` so the table entry
// feeds `vpermd`/`vpermps` without a copy. Output dword `j` reads input
// dword `idx[j]`. Inactive output slots stay 0 in the table and the caller
// zeros them after the permute.
struct alignas(32) compress_indices_8x32 {
   unsigned int idx[8];
};

struct compress_lookup_table_8x32_storage {
   compress_indices_8x32 entry[256];
};

[[nodiscard]]
constexpr auto
build_compress_lookup_table_8x32() -> compress_lookup_table_8x32_storage {
   compress_lookup_table_8x32_storage out{};
   for (__SIZE_TYPE__ m = 0u; m < 256u; ++m) {
      __SIZE_TYPE__ k = 0u;
      for (__SIZE_TYPE__ i = 0u; i < 8u; ++i) {
         if (((m >> i) & 1u) != 0u) {
            out.entry[m].idx[k] = static_cast<unsigned int>(i);
            ++k;
         }
      }
   }
   return out;
}

inline constexpr compress_lookup_table_8x32_storage compress_lookup_table_8x32 =
   build_compress_lookup_table_8x32();

struct compress_lookup_table_4x64_storage {
   compress_indices_8x32 entry[16];
};

// AVX2 has no variable-index `vpermq`, so each 8-byte output lane is built
// from two 4-byte indices that target the low and high halves of the
// selected source qword. `vpermd` then handles the move.
[[nodiscard]]
constexpr auto
build_compress_lookup_table_4x64() -> compress_lookup_table_4x64_storage {
   compress_lookup_table_4x64_storage out{};
   for (__SIZE_TYPE__ m = 0u; m < 16u; ++m) {
      __SIZE_TYPE__ k = 0u;
      for (__SIZE_TYPE__ i = 0u; i < 4u; ++i) {
         if (((m >> i) & 1u) != 0u) {
            out.entry[m].idx[k * 2u] = static_cast<unsigned int>(i * 2u);
            out.entry[m].idx[(k * 2u) + 1u] =
               static_cast<unsigned int>((i * 2u) + 1u);
            ++k;
         }
      }
   }
   return out;
}

inline constexpr compress_lookup_table_4x64_storage compress_lookup_table_4x64 =
   build_compress_lookup_table_4x64();

// View of a 32-byte AVX2 vector as four 8-byte chunks for the `pdep`/`pext`
// path. The vector is bit-cast in and the stack buffer is bit-cast back out.
struct alignas(32) compress_chunks_4x64 {
   __UINT64_TYPE__ q[4];
};

struct alignas(32) compress_byte_buffer_32 {
   unsigned char b[32];
};

}  // namespace compress_detail

// 8-lane 4-byte AVX2 compress. Covers `int`, `unsigned`, and `float` on
// `avx_abi`. `vmovmskps` collapses the lane mask to 8 bits, the lookup
// table yields an 8-index permutation, `vpermd`/`vpermps` rearranges the
// input, then the trailing inactive lanes are zeroed.
template <typename T, x64::is_avx_abi<T> Abi>
   requires(sizeof(T) == 4u)
struct compress<T, Abi> {
   [[nodiscard, gnu::target("avx2"), gnu::nodebug]]
   static constexpr auto
   invoke(simd<T, Abi> input, simd_mask<T, Abi> selector) -> simd<T, Abi> {
      using v8si = simd<int, ::x64::avx_abi<int>>::raw_type;
      using raw_t = simd<T, Abi>::raw_type;

      __UINT32_TYPE__ const lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(selector);
      v8si const indices = __builtin_bit_cast(
         v8si, compress_detail::compress_lookup_table_8x32.entry[lane_bits]);

      int const active_count = __builtin_popcountg(lane_bits);
      v8si const count_splat = {
         active_count, active_count, active_count, active_count,
         active_count, active_count, active_count, active_count,
      };
      v8si const ascending = {0, 1, 2, 3, 4, 5, 6, 7};
      v8si const tail_mask = count_splat > ascending;

      if constexpr (is_floating_point<T>) {
         using v8sf = simd<float, ::x64::avx_abi<float>>::raw_type;
         v8sf const input_v8sf = __builtin_bit_cast(v8sf, input.raw);
         v8sf const permuted = __builtin_ia32_permvarsf256(input_v8sf, indices);
         v8si const masked = __builtin_bit_cast(v8si, permuted) & tail_mask;
         return simd<T, Abi>(__builtin_bit_cast(raw_t, masked));
      } else {
         v8si const input_v8si = __builtin_bit_cast(v8si, input.raw);
         v8si const permuted = __builtin_ia32_permvarsi256(input_v8si, indices);
         v8si const masked = permuted & tail_mask;
         return simd<T, Abi>(__builtin_bit_cast(raw_t, masked));
      }
   }
};

// 4-lane 8-byte AVX2 compress. Covers `long`, `unsigned long`, and `double`
// on `avx_abi`. AVX2 lacks a variable-index `vpermq`, so the permute runs as
// 8x4-byte dwords via `vpermd` with index pairs, and the tail mask
// duplicates each count bit across the dword pair.
template <typename T, x64::is_avx_abi<T> Abi>
   requires(sizeof(T) == 8u)
struct compress<T, Abi> {
   [[nodiscard, gnu::target("avx2"), gnu::nodebug]]
   static constexpr auto
   invoke(simd<T, Abi> input, simd_mask<T, Abi> selector) -> simd<T, Abi> {
      using v8si = simd<int, ::x64::avx_abi<int>>::raw_type;
      using raw_t = simd<T, Abi>::raw_type;

      __UINT32_TYPE__ const lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(selector);
      v8si const indices = __builtin_bit_cast(
         v8si, compress_detail::compress_lookup_table_4x64.entry[lane_bits]);

      int const active_count = __builtin_popcountg(lane_bits);
      v8si const count_splat = {
         active_count, active_count, active_count, active_count,
         active_count, active_count, active_count, active_count,
      };
      v8si const pair_index = {0, 0, 1, 1, 2, 2, 3, 3};
      v8si const tail_mask = count_splat > pair_index;

      v8si const input_v8si = __builtin_bit_cast(v8si, input.raw);
      v8si const permuted = __builtin_ia32_permvarsi256(input_v8si, indices);
      v8si const masked = permuted & tail_mask;
      return simd<T, Abi>(__builtin_bit_cast(raw_t, masked));
   }
};

// 16-lane 2-byte AVX2 compress. Covers `short`, `unsigned short`, and any
// other 2-byte lane on `avx_abi`. Splits the vector into four 8-byte chunks
// of 4 word lanes each. For each chunk: `pdep` spreads the 4-bit submask to
// 2-byte element stripes, `pext` packs the active words to the chunk's low
// end, and the result writes to a stack buffer at a running byte offset
// driven by `popcount`. `pext` zero-pads above the active bits and the
// buffer is zero-initialized, so the buffer reloads as a correctly
// zero-tailed result without an extra mask step.
template <typename T, x64::is_avx_abi<T> Abi>
   requires(sizeof(T) == 2u)
struct compress<T, Abi> {
   [[nodiscard, gnu::target("avx2,bmi2"), gnu::nodebug]]
   static auto
   invoke(simd<T, Abi> input, simd_mask<T, Abi> selector) -> simd<T, Abi> {
      using raw_t = simd<T, Abi>::raw_type;

      __UINT32_TYPE__ lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(selector);
      // TODO: Clang had a regression in SIMD constant folding. Remove this asm
      // when possible.
      asm volatile("" : "+r"(lane_bits));

      compress_detail::compress_chunks_4x64 const in_chunks =
         __builtin_bit_cast(compress_detail::compress_chunks_4x64, input.raw);

      compress_detail::compress_byte_buffer_32 buffer{};
      unsigned int byte_offset = 0u;
      for (unsigned int c = 0u; c < 4u; ++c) {
         unsigned int const submask = (lane_bits >> (c * 4u)) & 0xFu;
         __UINT64_TYPE__ const word_bit_mask =
            __builtin_ia32_pdep_di(submask, 0x00010001'00010001ULL) * 0xFFFFULL;
         __UINT64_TYPE__ const compressed =
            __builtin_ia32_pext_di(in_chunks.q[c], word_bit_mask);
         __builtin_memcpy(&buffer.b[byte_offset], &compressed,
                          sizeof(compressed));
         byte_offset += __builtin_popcountg(submask) * 2u;
      }

      return simd<T, Abi>(__builtin_bit_cast(raw_t, buffer));
   }
};

// 32-lane 1-byte AVX2 compress. Covers `signed char`, `unsigned char`, and
// `char` on `avx_abi`. Same four-chunk `pdep`/`pext` recipe as the 2-byte
// path, with the submask widened to 8 bits per chunk and the `pdep`
// pattern striped at byte instead of word resolution.
template <typename T, x64::is_avx_abi<T> Abi>
   requires(sizeof(T) == 1u)
struct compress<T, Abi> {
   [[nodiscard, gnu::target("avx2,bmi2"), gnu::nodebug]]
   static auto
   invoke(simd<T, Abi> input, simd_mask<T, Abi> selector) -> simd<T, Abi> {
      using raw_t = simd<T, Abi>::raw_type;

      __UINT32_TYPE__ const lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(selector);

      compress_detail::compress_chunks_4x64 const in_chunks =
         __builtin_bit_cast(compress_detail::compress_chunks_4x64, input.raw);

      compress_detail::compress_byte_buffer_32 buffer{};
      unsigned int byte_offset = 0u;
      for (unsigned int c = 0u; c < 4u; ++c) {
         unsigned int const submask = (lane_bits >> (c * 8u)) & 0xFFu;
         __UINT64_TYPE__ const byte_bit_mask =
            __builtin_ia32_pdep_di(submask, 0x01010101'01010101ULL) * 0xFFULL;
         __UINT64_TYPE__ const compressed =
            __builtin_ia32_pext_di(in_chunks.q[c], byte_bit_mask);
         __builtin_memcpy(&buffer.b[byte_offset], &compressed,
                          sizeof(compressed));
         byte_offset += __builtin_popcountg(submask);
      }

      return simd<T, Abi>(__builtin_bit_cast(raw_t, buffer));
   }
};

}  // namespace cat::detail::simd_abi
