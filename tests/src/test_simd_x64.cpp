#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/simd>
#include <cat/simd_ops>

#include "../unit_tests.hpp"

using cat::float4x4;
using cat::int4x4;
using cat::int4x8;

namespace {

// Differential check helper for the AVX2 `simd_compress` hooks. The AVX path
// in `simd_avx2_compress.hpp` runs against the scalar fallback from
// `simd_compress_impl` on a same-width `fixed_size` reference vector, lane by
// lane. Any divergence (mask bit pattern, LUT index, BMI2 `pext` byte
// offset, tail zero) surfaces as a lane mismatch.
template <
   typename AvxSimd, typename AvxMask, typename RefSimd, typename RefMask>
[[gnu::always_inline]]
auto
verify_avx2_compress_matches_reference(
   AvxSimd const& avx_input, RefSimd const& ref_input, AvxMask const& avx_mask,
   RefMask const& ref_mask
) -> void {
   auto const avx_packed = cat::simd_compress(avx_input, avx_mask);
   auto const ref_packed = cat::simd_compress(ref_input, ref_mask);
   for (cat::idx i = 0u; i < AvxSimd::abi_type::lanes; ++i) {
      cat::verify(avx_packed[i] == ref_packed[i]);
   }
}

[[nodiscard]]
auto
has_avx512_runtime_support() -> bool {
   return __builtin_cpu_supports("avx512f")
          && __builtin_cpu_supports("avx512cd")
          && __builtin_cpu_supports("avx512bw")
          && __builtin_cpu_supports("avx512dq")
          && __builtin_cpu_supports("avx512vl");
}

[[gnu::target("avx512f,avx512cd,avx512bw,avx512dq,avx512vl")]]
auto
verify_avx512_abi_runtime_hooks() -> void {
   x64::avx512_simd<cat::uint8> const words{1u,  3u,  7u,   15u,
                                            31u, 63u, 127u, cat::uint8::max()};
   auto const counts = words.popcount();
   cat::verify(counts[0u] == 1u);
   cat::verify(counts[1u] == 2u);
   cat::verify(counts[7u] == 64u);
   cat::verify(counts.sum() == 92u);

   x64::avx512_simd<cat::uint4> const lanes{
      1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u
   };
   x64::avx512_simd<cat::uint4> const threshold{8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
                                                8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u};
   auto const high_lanes = lanes > threshold;
   cat::verify(high_lanes.count_if_true() == 8u);
   cat::verify(high_lanes.find_if_true() == 8u);
   cat::verify(high_lanes.find_last_if_true() == 15u);

   cat::bitset<16u> const bits = high_lanes.to_bitset();
   for (cat::idx i = 0u; i < 8u; ++i) {
      cat::verify(!bits[i]);
   }
   for (cat::idx i = 8u; i < 16u; ++i) {
      cat::verify(bits[i]);
   }
}

}  // namespace

// SSE hooks (`simd_sse.hpp`).
// `unary_full<op_rsqrt>`, plus `mask_to_bitset` via `sse2_abi_mask_to_bitset`
// in `simd_mask_bitset.tpp`. Reference behavior uses `simd_abi::fixed_size`
// without ISA hooks, not `simd_abi::native` which may alias `avx_abi` on this
// target.
$test(simd_sse_abi_hooks_permute_and_rsqrt) {
   static_assert(cat::simd_abi::scalar<int4>::size == sizeof(int4));
   static_assert(cat::simd_abi::scalar<int4>::alignment == alignof(int4));
   static_assert(cat::simd_abi::scalar<float4>::lanes == 1u);

   x64::sse_simd<int4> const ints{10, 20, 30, 40};
   cat::fixed_size_simd<int4, 4u> const ref_ints{10, 20, 30, 40};
   int4x4 const irev{3u, 2u, 1u, 0u};
   cat::verify(cat::simd_permute(ints, irev)[0u] == 40);
   cat::verify(cat::simd_permute(ints, irev)[3u] == 10);
   cat::verify(
      cat::simd_permute(ref_ints, irev)
      == static_cast<cat::fixed_size_simd<int4, 4u>>(
         cat::simd_permute(ints, irev)
      )
   );

   int4x4 const iid{0u, 1u, 2u, 3u};
   cat::verify(cat::simd_permute(ints, iid) == ints);
   cat::verify(cat::simd_permute(ref_ints, iid) == ref_ints);

   x64::sse_simd<float4> const floats{10_f4, 20_f4, 30_f4, 40_f4};
   cat::fixed_size_simd<float4, 4u> const ref_floats{
      10_f4, 20_f4, 30_f4, 40_f4
   };
   float4x4 const frev{3_f4, 2_f4, 1_f4, 0_f4};
   cat::verify(cat::simd_permute(floats, frev)[0u] == 40_f4);
   cat::verify(
      cat::simd_permute(ref_floats, frev)
      == static_cast<cat::fixed_size_simd<float4, 4u>>(
         cat::simd_permute(floats, frev)
      )
   );

   float4x4 const fid{0_f4, 1_f4, 2_f4, 3_f4};
   cat::verify(cat::simd_permute(floats, fid) == floats);

   x64::sse_simd<float4> const unit{4_f4, 4_f4, 4_f4, 4_f4};
   cat::fixed_size_simd<float4, 4u> const unit_ref{4_f4, 4_f4, 4_f4, 4_f4};
   auto const rr = cat::simd_rsqrt(unit);
   cat::fixed_size_simd<float4, 4u> const rr_scalar = cat::simd_rsqrt(unit_ref);
   for (idx i = 0u; i < idx{4}; ++i) {
      cat::verify(cat::abs(rr[i] - 0.5_f4) < 0.001_f4);
      cat::verify(cat::abs(rr[i] - rr_scalar[i]) < 0.002_f4);
   }
}

$test(simd_sse_abi_hooks_mask_to_bitset) {
   x64::sse_simd<float> const v{1.f, 2.f, 3.f, 4.f};
   auto const m = v > x64::sse_simd<float>{2.f, 2.f, 2.f, 2.f};
   cat::bitset<4u> const bits = m.to_bitset();
   cat::verify(bits[0u] == false);
   cat::verify(bits[1u] == false);
   cat::verify(bits[2u] == true);
   cat::verify(bits[3u] == true);
}

// AVX hooks (`simd_avx2.hpp`).
// `unary_full<op_rsqrt>`, plus mask reductions via `simd_avx2_mask_ops.hpp` and
// `simd_avx2_mask_ops.hpp` mask reductions. `mask_to_bitset` via
// `avx2_abi_mask_to_bitset`.
$test(simd_avx_abi_hooks_permute_and_rsqrt) {
   x64::avx_simd<float4> const fv{10_f4, 20_f4, 30_f4, 40_f4,
                                  50_f4, 60_f4, 70_f4, 80_f4};
   int4x8 const fid{0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u};
   cat::verify(cat::simd_permute(fv, fid) == fv);

   cat::fixed_size_simd<float4, 4u> const flo{10_f4, 20_f4, 30_f4, 40_f4};
   int4x4 const rev4{3u, 2u, 1u, 0u};
   int4x8 const rev_low{3u, 2u, 1u, 0u, 4u, 5u, 6u, 7u};
   auto const perm_avx = cat::simd_permute(fv, rev_low);
   cat::verify(
      cat::simd_permute(flo, rev4)
      == cat::fixed_size_simd<float4, 4u>(
         perm_avx[0u], perm_avx[1u], perm_avx[2u], perm_avx[3u]
      )
   );

   int4x8 const fbroadcast{};
   cat::verify(cat::simd_permute(fv, fbroadcast)[7u] == fv[0u]);

   x64::avx_simd<int4> const iv{1, 2, 3, 4, 5, 6, 7, 8};
   cat::verify(cat::simd_permute(iv, fid) == iv);
   cat::fixed_size_simd<int4, 4u> const ilo{1, 2, 3, 4};
   int4x4 const irev4{3u, 2u, 1u, 0u};
   int4x8 const irev_low{3u, 2u, 1u, 0u, 4u, 5u, 6u, 7u};
   auto const iperm = cat::simd_permute(iv, irev_low);
   cat::verify(
      cat::simd_permute(ilo, irev4)
      == cat::fixed_size_simd<int4, 4u>(
         iperm[0u], iperm[1u], iperm[2u], iperm[3u]
      )
   );

   x64::avx_simd<double> const dv{1., 2., 3., 4.};
   int4x4 const did{0u, 1u, 2u, 3u};
   cat::verify(cat::simd_permute(dv, did) == dv);

   int4x4 const drev{3u, 2u, 1u, 0u};
   auto const swapped = cat::simd_permute(dv, drev);
   cat::verify(swapped[0u] == 4.);
   cat::verify(swapped[3u] == 1.);
   cat::fixed_size_simd<double, 2u> const pair_front{4., 3.};
   cat::fixed_size_simd<double, 2u> const pair_back{2., 1.};
   cat::verify(swapped[0u] == pair_front[0u]);
   cat::verify(swapped[1u] == pair_front[1u]);
   cat::verify(swapped[2u] == pair_back[0u]);
   cat::verify(swapped[3u] == pair_back[1u]);

   x64::avx_simd<float4> ones{};
   ones.fill(4_f4);
   cat::fixed_size_simd<float4, 4u> ones_ref{};
   ones_ref.fill(4_f4);
   auto const rr = cat::simd_rsqrt(ones);
   cat::fixed_size_simd<float4, 4u> const rr_scalar = cat::simd_rsqrt(ones_ref);
   for (idx i = 0u; i < idx{8}; ++i) {
      idx const j = idx{static_cast<uword>(i.raw % 4u)};
      cat::verify(cat::abs(rr[i] - 0.5_f4) < 0.001_f4);
      cat::verify(cat::abs(rr[i] - rr_scalar[j]) < 0.002_f4);
   }
}

// Exercises the hardware `vrsqrtps`/`vrsqrtps256` path with one Newton-Raphson
// refinement step. Bare hardware is only ~12-bit (<= 1.5 * 2^-12, about
// 3.7e-4 relative error). After the NR step we expect ~23-bit
// (single-precision) accuracy. A tolerance of 1e-5 would fail for the
// unrefined hook and pass with the refined one, so this is a regression check
// that NR is on.
$test(simd_sse_rsqrt_fast_is_nr_refined) {
   using sse_fast = cat::simd<cat::float4_fast, x64::sse_abi<cat::float4_fast>>;
   sse_fast const v{
      cat::float4_fast(4.f), cat::float4_fast(0.25f), cat::float4_fast(16.f),
      cat::float4_fast(100.f)
   };
   auto const rr = cat::simd_rsqrt(v);
   float const expected[4] = {0.5f, 2.f, 0.25f, 0.1f};
   for (cat::idx i = 0u; i < cat::idx{4}; ++i) {
      float const got = static_cast<float>(rr[i]);
      cat::verify(cat::abs(got - expected[i.raw]) < 1e-5f * expected[i.raw]);
   }
}

$test(simd_avx_rsqrt_fast_is_nr_refined) {
   using avx_fast = cat::simd<cat::float4_fast, x64::avx_abi<cat::float4_fast>>;
   avx_fast const v{cat::float4_fast(4.f),     cat::float4_fast(0.25f),
                    cat::float4_fast(16.f),    cat::float4_fast(100.f),
                    cat::float4_fast(0.0625f), cat::float4_fast(625.f),
                    cat::float4_fast(2.f),     cat::float4_fast(9.f)};
   auto const rr = cat::simd_rsqrt(v);
   float const expected[8] = {0.5f, 2.f,   0.25f,       0.1f,
                              4.f,  0.04f, 0.70710677f, 1.f / 3.f};
   for (cat::idx i = 0u; i < cat::idx{8}; ++i) {
      float const got = static_cast<float>(rr[i]);
      cat::verify(cat::abs(got - expected[i.raw]) < 1e-5f * expected[i.raw]);
   }
}

$test(simd_avx_rsqrt_times_sqrt_near_one) {
   using avxf = cat::simd<float, x64::avx_abi<float>>;
   using sxf = cat::fixed_size_simd<float, 4u>;
   avxf wide{};
   wide.fill(4.f);
   sxf narrow{};
   narrow.fill(4.f);
   avxf const wr = cat::simd_rsqrt(wide);
   avxf const s = cat::simd_sqrt(wide);
   sxf const wr_ref = cat::simd_rsqrt(narrow);
   sxf const s_ref = cat::simd_sqrt(narrow);
   for (idx i = 0u; i < avxf::abi_type::lanes; ++i) {
      float const p = wr[i] * s[i];
      idx const j = idx{static_cast<uword>(i.raw % 4u)};
      float const ref_prod = wr_ref[j] * s_ref[j];
      cat::verify(cat::abs(p - ref_prod) < 0.01f);
      cat::verify(cat::abs(p - 1.f) < 0.02f);
   }
}

$test(simd_avx_abi_hooks_mask_reductions_and_bitset) {
   x64::avx_simd_mask<float4> const sparse{false, false, true,  false,
                                           false, true,  false, true};
   // Eight `float4` lanes need 32 bytes. Compare against
   // `simd_abi::fixed_size<float4, 8>` instead of `simd_abi::native<float4>`
   // which aliases `avx_abi<float4>` here. No AVX2 mask hook specializations
   // apply to `simd_abi::fixed_size`.
   cat::simd_mask<float4, cat::simd_abi::fixed_size<float4, 8>> const
      sparse_ref{false, false, true, false, false, true, false, true};
   cat::verify(sparse.count_if_true() == sparse_ref.count_if_true());
   cat::verify(sparse.find_if_true() == sparse_ref.find_if_true());
   cat::verify(sparse.find_last_if_true() == sparse_ref.find_last_if_true());

   x64::avx_simd_mask<float4> const all_true{true, true, true, true,
                                             true, true, true, true};
   cat::simd_mask<float4, cat::simd_abi::fixed_size<float4, 8>> const
      all_true_ref{true, true, true, true, true, true, true, true};
   cat::verify(all_true.count_if_true() == all_true_ref.count_if_true());
   cat::verify(cat::simd_all_of(all_true) == cat::simd_all_of(all_true_ref));
   cat::verify(all_true.find_if_true() == all_true_ref.find_if_true());
   cat::verify(
      all_true.find_last_if_true() == all_true_ref.find_last_if_true()
   );

   x64::avx_simd_mask<float4> const none{false, false, false, false,
                                         false, false, false, false};
   cat::simd_mask<float4, cat::simd_abi::fixed_size<float4, 8>> const none_ref{
      false, false, false, false, false, false, false, false
   };
   cat::verify(none.count_if_true() == none_ref.count_if_true());
   cat::verify(cat::simd_any_of(none) == cat::simd_any_of(none_ref));

   x64::avx_simd_mask<float4> const one{false, false, false, true,
                                        false, false, false, false};
   cat::simd_mask<float4, cat::simd_abi::fixed_size<float4, 8>> const one_ref{
      false, false, false, true, false, false, false, false
   };
   cat::verify(one.find_if_true() == one_ref.find_if_true());
   cat::verify(one.find_last_if_true() == one_ref.find_last_if_true());

   cat::simd_mask<float4, cat::simd_abi::fixed_size<float4, 4u>> const
      sparse_lo{sparse[0u], sparse[1u], sparse[2u], sparse[3u]};
   cat::simd_mask<float4, cat::simd_abi::fixed_size<float4, 4u>> const
      sparse_hi{sparse[4u], sparse[5u], sparse[6u], sparse[7u]};
   cat::verify(
      sparse.count_if_true()
      == sparse_lo.count_if_true() + sparse_hi.count_if_true()
   );

   cat::bitset<8u> const b = sparse.to_bitset();
   cat::bitset<8u> const b_ref = sparse_ref.to_bitset();
   for (idx k = 0u; k < 8u; ++k) {
      cat::verify(b[k] == b_ref[k]);
   }
   for (idx i = 0u; i < 8u; ++i) {
      cat::verify(b[i] == sparse[i]);
   }
}

$test(simd_avx512_abi_layout_and_runtime_hooks) {
   static_assert(x64::avx512_simd<cat::uint8>::size() == 8u);
   static_assert(x64::avx512_simd<cat::uint4>::size() == 16u);
   static_assert(x64::avx512_simd<cat::uint1>::size() == 64u);
   static_assert(alignof(x64::avx512_simd<cat::uint8>) == 64u);
   static_assert(x64::avx512_unaligned_simd<cat::uint8>::size() == 8u);
   static_assert(alignof(x64::avx512_unaligned_simd<cat::uint8>) == 1u);
   static_assert(x64::avx512_simd_mask<cat::uint4>::size() == 16u);

   if (has_avx512_runtime_support()) {
      verify_avx512_abi_runtime_hooks();
   }
}

$test(simd_avx_unaligned_abi) {
   static_assert(x64::avx_unaligned_simd<float>::size() == 8u);
   static_assert(alignof(x64::avx_unaligned_simd<float>) == 1);

   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(128_uki).or_exit();
   $defer {
      pager.free(page);
   };
   cat::linear_allocator allocator = cat::make_linear_allocator(page);

   // Misalign allocator by 2 bytes.
   auto* _ = allocator.alloc<short>().verify();
   // Allocate unaligned SIMD.
   auto* p_unaligned =
      allocator.unalign_alloc<x64::avx_unaligned_simd<float>>().verify();

   cat::verify(!cat::is_aligned(p_unaligned, 32_uz));
   cat::verify(!cat::is_aligned(p_unaligned, 16_uz));
   cat::verify(cat::is_aligned(p_unaligned, 2_uz));

   cat::verify(!cat::is_aligned(p_unaligned->data(), 32_uz));
   cat::verify(!cat::is_aligned(p_unaligned->data(), 16_uz));
   cat::verify(cat::is_aligned(p_unaligned->data(), 2_uz));

   p_unaligned->fill(4.f);

   cat::verify(p_unaligned[0u] == 4.f);

   auto const rr = cat::simd_rsqrt(*p_unaligned);
   cat::verify(cat::abs(rr[0u] - 0.5_f4) < 0.001_f4);
}

$test(simd_sse_and_unaligned_abi) {
   static_assert(x64::sse_unaligned_simd<float>::size() == 4u);
   static_assert(alignof(x64::sse_unaligned_simd<float>) == 1);

   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(128_uki).or_exit();
   $defer {
      pager.free(page);
   };
   cat::linear_allocator allocator = cat::make_linear_allocator(page);

   // Misalign allocator by 2 bytes.
   auto* _ = allocator.alloc<short>().verify();
   // Allocate unaligned SIMD.
   auto* p_unaligned =
      allocator.unalign_alloc<x64::sse_unaligned_simd<float>>().verify();

   cat::verify(!cat::is_aligned(p_unaligned, 16_uz));
   cat::verify(cat::is_aligned(p_unaligned, 2_uz));

   cat::verify(!cat::is_aligned(p_unaligned->data(), 16_uz));
   cat::verify(cat::is_aligned(p_unaligned->data(), 2_uz));

   p_unaligned->fill(4.f);

   cat::verify(p_unaligned[0u] == 4.f);

   auto const rr = cat::simd_rsqrt(*p_unaligned);
   cat::verify(cat::abs(rr[0u] - 0.5_f4) < 0.001_f4);
}

$test(simd_avx_unaligned_abi_mask) {
   static_assert(x64::avx_unaligned_simd_mask<float>::size() == 8u);
   static_assert(alignof(x64::avx_unaligned_simd_mask<float>) == 1);

   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(128_uki).or_exit();
   $defer {
      pager.free(page);
   };
   cat::linear_allocator allocator = cat::make_linear_allocator(page);

   // Misalign allocator by 2 bytes.
   auto* _ = allocator.alloc<short>().verify();
   // Allocate unaligned mask.
   auto* p_unaligned =
      allocator.unalign_alloc<x64::avx_unaligned_simd_mask<float>>().verify();

   cat::verify(!cat::is_aligned(p_unaligned, 32_uz));
   cat::verify(!cat::is_aligned(p_unaligned, 16_uz));
   cat::verify(cat::is_aligned(p_unaligned, 2_uz));

   cat::verify(!cat::is_aligned(p_unaligned->data(), 32_uz));
   cat::verify(!cat::is_aligned(p_unaligned->data(), 16_uz));
   cat::verify(cat::is_aligned(p_unaligned->data(), 2_uz));

   p_unaligned->fill(true);
   cat::verify((*p_unaligned)[0u]);
   cat::verify(cat::simd_all_of(*p_unaligned));

   p_unaligned->fill(false);
   cat::verify(!cat::simd_any_of(*p_unaligned));
}

$test(simd_sse_and_unaligned_abi_mask) {
   static_assert(x64::sse_unaligned_simd_mask<float>::size() == 4u);
   static_assert(alignof(x64::sse_unaligned_simd_mask<float>) == 1);

   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(128_uki).or_exit();
   $defer {
      pager.free(page);
   };
   cat::linear_allocator allocator = cat::make_linear_allocator(page);

   // Misalign allocator by 2 bytes.
   auto* _ = allocator.alloc<short>().verify();
   // Allocate unaligned mask.
   auto* p_unaligned =
      allocator.unalign_alloc<x64::sse_unaligned_simd_mask<float>>().verify();

   cat::verify(!cat::is_aligned(p_unaligned, 16_uz));
   cat::verify(cat::is_aligned(p_unaligned, 2_uz));

   cat::verify(!cat::is_aligned(p_unaligned->data(), 16_uz));
   cat::verify(cat::is_aligned(p_unaligned->data(), 2_uz));

   p_unaligned->fill(true);
   cat::verify((*p_unaligned)[0u]);
   cat::verify(cat::simd_all_of(*p_unaligned));

   p_unaligned->fill(false);
   cat::verify(!cat::simd_any_of(*p_unaligned));
}

$test(simd_unaligned_abi_adaptor) {
   static_assert(cat::is_same<
                 x64::avx_unaligned_abi<float>,
                 cat::simd_abi::unaligned<x64::avx_abi<float>>>);
   static_assert(cat::is_same<
                 x64::sse_unaligned_abi<float>,
                 cat::simd_abi::unaligned<x64::sse_abi<float>>>);

   static_assert(
      cat::simd_abi::unaligned<cat::simd_abi::native<float>>::lanes
      == cat::simd_abi::native<float>::lanes
   );
   static_assert(
      cat::simd_abi::unaligned<cat::simd_abi::native<float>>::size
      == cat::simd_abi::native<float>::size
   );
   static_assert(
      cat::simd_abi::unaligned<cat::simd_abi::native<float>>::alignment == 1u
   );

   static_assert(alignof(cat::native_unaligned_simd<float>) == 1u);
   static_assert(
      alignof(cat::fixed_size_unaligned_simd<cat::float4, 4u>) == 1u
   );
   static_assert(alignof(cat::scalar_unaligned_simd<cat::float4>) == 1u);
   static_assert(alignof(cat::compatible_unaligned_simd<float>) == 1u);
}

// AVX2 `simd_compress` hooks (`simd_avx2_compress.hpp`). One test per lane
// width covers the four specializations: 32-bit and 64-bit go through the
// `vpermd`/`vpermps` permutation-table path, 16-bit and 8-bit go through the
// BMI2 `pdep`/`pext` per-chunk path. Each test exercises a small bank of
// mask patterns and compares lane-by-lane against the scalar fallback on a
// `fixed_size` reference of the same width.

$test(simd_avx2_compress_dword) {
   using int_avx = x64::avx_simd<cat::int4>;
   using int_avx_mask = x64::avx_simd_mask<cat::int4>;
   using int_ref = cat::fixed_size_simd<cat::int4, 8u>;
   using int_ref_mask =
      cat::simd_mask<cat::int4, cat::simd_abi::fixed_size<cat::int4, 8u>>;

   int_avx const int_input{1, 2, 3, 4, 5, 6, 7, 8};
   int_ref const int_input_ref{1, 2, 3, 4, 5, 6, 7, 8};

   for (cat::idx active = 0u; active <= 8u; ++active) {
      verify_avx2_compress_matches_reference(
         int_input, int_input_ref,
         cat::make_simd_mask_from_count<int_avx>(active),
         cat::make_simd_mask_from_count<int_ref>(active)
      );
   }

   int_avx_mask const sparse{false, true,  false, true,
                             true,  false, true,  false};
   int_ref_mask const sparse_ref{false, true,  false, true,
                                 true,  false, true,  false};
   verify_avx2_compress_matches_reference(
      int_input, int_input_ref, sparse, sparse_ref
   );

   int_avx_mask const last_only{false, false, false, false,
                                false, false, false, true};
   int_ref_mask const last_only_ref{false, false, false, false,
                                    false, false, false, true};
   verify_avx2_compress_matches_reference(
      int_input, int_input_ref, last_only, last_only_ref
   );

   auto const all_true = cat::simd_compress(
      int_input, cat::make_simd_mask_from_count<int_avx>(8u)
   );
   cat::verify(all_true[0u] == 1);
   cat::verify(all_true[7u] == 8);

   auto const sparse_packed = cat::simd_compress(int_input, sparse);
   cat::verify(sparse_packed[0u] == 2);
   cat::verify(sparse_packed[1u] == 4);
   cat::verify(sparse_packed[2u] == 5);
   cat::verify(sparse_packed[3u] == 7);
   cat::verify(sparse_packed[4u] == 0);
   cat::verify(sparse_packed[7u] == 0);

   using float_avx = x64::avx_simd<cat::float4>;
   using float_avx_mask = x64::avx_simd_mask<cat::float4>;
   using float_ref = cat::fixed_size_simd<cat::float4, 8u>;
   using float_ref_mask =
      cat::simd_mask<cat::float4, cat::simd_abi::fixed_size<cat::float4, 8u>>;

   float_avx const float_input{1_f4, 2_f4, 3_f4, 4_f4, 5_f4, 6_f4, 7_f4, 8_f4};
   float_ref const float_input_ref{1_f4, 2_f4, 3_f4, 4_f4,
                                   5_f4, 6_f4, 7_f4, 8_f4};

   for (cat::idx active = 0u; active <= 8u; ++active) {
      verify_avx2_compress_matches_reference(
         float_input, float_input_ref,
         cat::make_simd_mask_from_count<float_avx>(active),
         cat::make_simd_mask_from_count<float_ref>(active)
      );
   }

   float_avx_mask const float_sparse{true,  false, false, true,
                                     false, true,  true,  false};
   float_ref_mask const float_sparse_ref{true,  false, false, true,
                                         false, true,  true,  false};
   verify_avx2_compress_matches_reference(
      float_input, float_input_ref, float_sparse, float_sparse_ref
   );

   auto const float_packed = cat::simd_compress(float_input, float_sparse);
   cat::verify(float_packed[0u] == 1_f4);
   cat::verify(float_packed[1u] == 4_f4);
   cat::verify(float_packed[2u] == 6_f4);
   cat::verify(float_packed[3u] == 7_f4);
   cat::verify(float_packed[4u] == 0_f4);
}

$test(simd_avx2_compress_qword) {
   using int_avx = x64::avx_simd<cat::int8>;
   using int_avx_mask = x64::avx_simd_mask<cat::int8>;
   using int_ref = cat::fixed_size_simd<cat::int8, 4u>;
   using int_ref_mask =
      cat::simd_mask<cat::int8, cat::simd_abi::fixed_size<cat::int8, 4u>>;

   int_avx const int_input{10, 20, 30, 40};
   int_ref const int_input_ref{10, 20, 30, 40};

   for (cat::idx active = 0u; active <= 4u; ++active) {
      verify_avx2_compress_matches_reference(
         int_input, int_input_ref,
         cat::make_simd_mask_from_count<int_avx>(active),
         cat::make_simd_mask_from_count<int_ref>(active)
      );
   }

   int_avx_mask const checker{true, false, true, false};
   int_ref_mask const checker_ref{true, false, true, false};
   verify_avx2_compress_matches_reference(
      int_input, int_input_ref, checker, checker_ref
   );

   int_avx_mask const high_only{false, false, false, true};
   int_ref_mask const high_only_ref{false, false, false, true};
   verify_avx2_compress_matches_reference(
      int_input, int_input_ref, high_only, high_only_ref
   );

   auto const checker_packed = cat::simd_compress(int_input, checker);
   cat::verify(checker_packed[0u] == 10);
   cat::verify(checker_packed[1u] == 30);
   cat::verify(checker_packed[2u] == 0);
   cat::verify(checker_packed[3u] == 0);

   using double_avx = x64::avx_simd<cat::float8>;
   using double_avx_mask = x64::avx_simd_mask<cat::float8>;
   using double_ref = cat::fixed_size_simd<cat::float8, 4u>;
   using double_ref_mask =
      cat::simd_mask<cat::float8, cat::simd_abi::fixed_size<cat::float8, 4u>>;

   double_avx const double_input{1.5, 2.5, 3.5, 4.5};
   double_ref const double_input_ref{1.5, 2.5, 3.5, 4.5};

   for (cat::idx active = 0u; active <= 4u; ++active) {
      verify_avx2_compress_matches_reference(
         double_input, double_input_ref,
         cat::make_simd_mask_from_count<double_avx>(active),
         cat::make_simd_mask_from_count<double_ref>(active)
      );
   }

   double_avx_mask const inner{false, true, true, false};
   double_ref_mask const inner_ref{false, true, true, false};
   verify_avx2_compress_matches_reference(
      double_input, double_input_ref, inner, inner_ref
   );
   auto const inner_packed = cat::simd_compress(double_input, inner);
   cat::verify(inner_packed[0u] == 2.5);
   cat::verify(inner_packed[1u] == 3.5);
   cat::verify(inner_packed[2u] == 0.0);
}

$test(simd_avx2_compress_word) {
   using word_avx = x64::avx_simd<cat::int2>;
   using word_avx_mask = x64::avx_simd_mask<cat::int2>;
   using word_ref = cat::fixed_size_simd<cat::int2, 16u>;
   using word_ref_mask =
      cat::simd_mask<cat::int2, cat::simd_abi::fixed_size<cat::int2, 16u>>;

   word_avx const input{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
   word_ref const input_ref{1, 2,  3,  4,  5,  6,  7,  8,
                            9, 10, 11, 12, 13, 14, 15, 16};

   for (cat::idx active = 0u; active <= 16u; ++active) {
      verify_avx2_compress_matches_reference(
         input, input_ref, cat::make_simd_mask_from_count<word_avx>(active),
         cat::make_simd_mask_from_count<word_ref>(active)
      );
   }

   // Alternating: each 4-lane `pext` chunk holds two active words.
   word_avx_mask const alternating{true, false, true, false, true, false,
                                   true, false, true, false, true, false,
                                   true, false, true, false};
   word_ref_mask const alternating_ref{true, false, true, false, true, false,
                                       true, false, true, false, true, false,
                                       true, false, true, false};
   verify_avx2_compress_matches_reference(
      input, input_ref, alternating, alternating_ref
   );

   // Cross-chunk: only the second chunk (lanes 4..7) is active.
   word_avx_mask const second_chunk{false, false, false, false, true,  true,
                                    true,  true,  false, false, false, false,
                                    false, false, false, false};
   word_ref_mask const second_chunk_ref{false, false, false, false,
                                        true,  true,  true,  true,
                                        false, false, false, false,
                                        false, false, false, false};
   verify_avx2_compress_matches_reference(
      input, input_ref, second_chunk, second_chunk_ref
   );

   // Single late lane: stresses the running byte-offset propagation.
   word_avx_mask const sparse_late{false, false, false, false, false, false,
                                   false, false, false, false, false, true,
                                   false, false, false, false};
   word_ref_mask const sparse_late_ref{false, false, false, false, false, false,
                                       false, false, false, false, false, true,
                                       false, false, false, false};
   verify_avx2_compress_matches_reference(
      input, input_ref, sparse_late, sparse_late_ref
   );

   auto const alt_packed = cat::simd_compress(input, alternating);
   cat::verify(alt_packed[0u] == 1);
   cat::verify(alt_packed[1u] == 3);
   cat::verify(alt_packed[7u] == 15);
   cat::verify(alt_packed[8u] == 0);
   cat::verify(alt_packed[15u] == 0);

   auto const late_packed = cat::simd_compress(input, sparse_late);
   cat::verify(late_packed[0u] == 12);
   cat::verify(late_packed[1u] == 0);
   cat::verify(late_packed[15u] == 0);
}

$test(simd_avx2_compress_byte) {
   using byte_avx = x64::avx_simd<cat::int1>;
   using byte_avx_mask = x64::avx_simd_mask<cat::int1>;
   using byte_ref = cat::fixed_size_simd<cat::int1, 32u>;
   using byte_ref_mask =
      cat::simd_mask<cat::int1, cat::simd_abi::fixed_size<cat::int1, 32u>>;

   byte_avx input{};
   byte_ref input_ref{};
   for (cat::idx i = 0u; i < 32u; ++i) {
      input.set_lane(i, static_cast<cat::int1>(i + 1));
      input_ref.set_lane(i, static_cast<cat::int1>(i + 1));
   }

   for (cat::idx active : {
           cat::idx{0u},
           cat::idx{1u},
           cat::idx{7u},
           cat::idx{8u},
           cat::idx{15u},
           cat::idx{16u},
           cat::idx{23u},
           cat::idx{31u},
           cat::idx{32u},
        }) {
      verify_avx2_compress_matches_reference(
         input, input_ref, cat::make_simd_mask_from_count<byte_avx>(active),
         cat::make_simd_mask_from_count<byte_ref>(active)
      );
   }

   // Alternating across every 8-lane `pext` chunk.
   byte_avx_mask alternating{};
   byte_ref_mask alternating_ref{};
   for (cat::idx i = 0u; i < 32u; ++i) {
      bool const bit = (i.raw % 2u) == 0u;
      alternating.set_lane(i, bit);
      alternating_ref.set_lane(i, bit);
   }
   verify_avx2_compress_matches_reference(
      input, input_ref, alternating, alternating_ref
   );

   // Only the third 8-byte chunk (lanes 16..23) is active.
   byte_avx_mask third_chunk{};
   byte_ref_mask third_chunk_ref{};
   for (cat::idx i = 16u; i < 24u; ++i) {
      third_chunk.set_lane(i, true);
      third_chunk_ref.set_lane(i, true);
   }
   verify_avx2_compress_matches_reference(
      input, input_ref, third_chunk, third_chunk_ref
   );

   // One active lane in the final chunk: exercises the maximum byte offset.
   byte_avx_mask tail_only{};
   byte_ref_mask tail_only_ref{};
   tail_only.set_lane(31u, true);
   tail_only_ref.set_lane(31u, true);
   verify_avx2_compress_matches_reference(
      input, input_ref, tail_only, tail_only_ref
   );

   auto const alt_packed = cat::simd_compress(input, alternating);
   cat::verify(alt_packed[0u] == cat::int1{1});
   cat::verify(alt_packed[1u] == cat::int1{3});
   cat::verify(alt_packed[15u] == cat::int1{31});
   cat::verify(alt_packed[16u] == cat::int1{0});
   cat::verify(alt_packed[31u] == cat::int1{0});

   auto const tail_packed = cat::simd_compress(input, tail_only);
   cat::verify(tail_packed[0u] == cat::int1{32});
   cat::verify(tail_packed[1u] == cat::int1{0});
   cat::verify(tail_packed[31u] == cat::int1{0});
}

// C++26 `[simd.permute.mask]` three-argument compress (P2664). The trailing
// inactive slots are filled with the caller-provided scalar instead of the
// two-argument form's zero. Covers the dword and qword AVX2 hooks where the
// tail comes from a SIMD AND-mask, and the word/byte BMI2 hooks where the
// tail comes from `pext` zero-padding. Both must blend correctly when the
// hook is dispatched through the `simd_select` step inside
// `simd_compress_impl`.
$test(simd_avx2_compress_with_fill_value) {
   using int_avx = x64::avx_simd<cat::int4>;
   using int_avx_mask = x64::avx_simd_mask<cat::int4>;
   int_avx const dword_input{1, 2, 3, 4, 5, 6, 7, 8};
   int_avx_mask const dword_first_three =
      cat::make_simd_mask_from_count<int_avx>(3u);
   auto const dword_filled =
      cat::simd_compress(dword_input, dword_first_three, cat::int4{-1});
   cat::verify(dword_filled[0u] == 1);
   cat::verify(dword_filled[1u] == 2);
   cat::verify(dword_filled[2u] == 3);
   cat::verify(dword_filled[3u] == -1);
   cat::verify(dword_filled[7u] == -1);

   using float_avx = x64::avx_simd<cat::float4>;
   using float_avx_mask = x64::avx_simd_mask<cat::float4>;
   float_avx const float_input{1_f4, 2_f4, 3_f4, 4_f4, 5_f4, 6_f4, 7_f4, 8_f4};
   float_avx_mask const float_sparse{true,  false, false, true,
                                     false, true,  true,  false};
   auto const float_filled =
      cat::simd_compress(float_input, float_sparse, 99_f4);
   cat::verify(float_filled[0u] == 1_f4);
   cat::verify(float_filled[1u] == 4_f4);
   cat::verify(float_filled[2u] == 6_f4);
   cat::verify(float_filled[3u] == 7_f4);
   cat::verify(float_filled[4u] == 99_f4);
   cat::verify(float_filled[7u] == 99_f4);

   using qword_avx = x64::avx_simd<cat::int8>;
   qword_avx const qword_input{10, 20, 30, 40};
   auto const qword_filled = cat::simd_compress(
      qword_input, cat::make_simd_mask_from_count<qword_avx>(1u), cat::int8{777}
   );
   cat::verify(qword_filled[0u] == 10);
   cat::verify(qword_filled[1u] == 777);
   cat::verify(qword_filled[3u] == 777);

   using word_avx = x64::avx_simd<cat::int2>;
   using word_avx_mask = x64::avx_simd_mask<cat::int2>;
   word_avx const word_input{1, 2,  3,  4,  5,  6,  7,  8,
                             9, 10, 11, 12, 13, 14, 15, 16};
   word_avx_mask const word_alternating{true, false, true, false, true, false,
                                        true, false, true, false, true, false,
                                        true, false, true, false};
   auto const word_filled =
      cat::simd_compress(word_input, word_alternating, cat::int2{-5});
   cat::verify(word_filled[0u] == 1);
   cat::verify(word_filled[7u] == 15);
   cat::verify(word_filled[8u] == -5);
   cat::verify(word_filled[15u] == -5);

   using byte_avx = x64::avx_simd<cat::int1>;
   byte_avx byte_input{};
   for (cat::idx i = 0u; i < 32u; ++i) {
      byte_input.set_lane(i, static_cast<cat::int1>(i + 1));
   }
   auto const byte_filled = cat::simd_compress(
      byte_input, cat::make_simd_mask_from_count<byte_avx>(5u), cat::int1{42}
   );
   cat::verify(byte_filled[0u] == cat::int1{1});
   cat::verify(byte_filled[4u] == cat::int1{5});
   cat::verify(byte_filled[5u] == cat::int1{42});
   cat::verify(byte_filled[31u] == cat::int1{42});
}
