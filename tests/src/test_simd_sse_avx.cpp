#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/simd>
#include <cat/simd_ops>

#include "../unit_tests.hpp"

using cat::float4x4;
using cat::int4x4;
using cat::int4x8;

// SSE2 hooks (`simd_sse2.hpp`).
// `unary_full<op_rsqrt>`, plus `mask_to_bitset` via `sse2_abi_mask_to_bitset`
// in `simd_mask_bitset.tpp`. Reference behavior uses `scalar_abi` (16-byte, no
// ISA hooks), not `native_abi` (which may alias `avx2_abi` on this target).
$test(simd_sse2_abi_hooks_permute_and_rsqrt) {
   static_assert(cat::scalar_abi<int4>::size == 16u);
   static_assert(cat::scalar_abi<int4>::alignment == 16u);
   static_assert(cat::scalar_abi<float4>::lanes == idx{4});

   x64::sse2_simd<int4> const ints{10, 20, 30, 40};
   cat::scalar_simd<int4> const ref_ints{10, 20, 30, 40};
   int4x4 const irev{3u, 2u, 1u, 0u};
   cat::verify(cat::simd_permute(ints, irev)[0u] == 40);
   cat::verify(cat::simd_permute(ints, irev)[3u] == 10);
   cat::verify(
      cat::simd_permute(ref_ints, irev)
      == static_cast<cat::scalar_simd<int4>>(cat::simd_permute(ints, irev)));

   int4x4 const iid{0u, 1u, 2u, 3u};
   cat::verify(cat::simd_permute(ints, iid) == ints);
   cat::verify(cat::simd_permute(ref_ints, iid) == ref_ints);

   x64::sse2_simd<float4> const floats{10_f4, 20_f4, 30_f4, 40_f4};
   cat::scalar_simd<float4> const ref_floats{10_f4, 20_f4, 30_f4, 40_f4};
   float4x4 const frev{3_f4, 2_f4, 1_f4, 0_f4};
   cat::verify(cat::simd_permute(floats, frev)[0u] == 40_f4);
   cat::verify(cat::simd_permute(ref_floats, frev)
               == static_cast<cat::scalar_simd<float4>>(
                  cat::simd_permute(floats, frev)));

   float4x4 const fid{0_f4, 1_f4, 2_f4, 3_f4};
   cat::verify(cat::simd_permute(floats, fid) == floats);

   x64::sse2_simd<float4> const unit{4_f4, 4_f4, 4_f4, 4_f4};
   cat::scalar_simd<float4> const unit_ref{4_f4, 4_f4, 4_f4, 4_f4};
   auto const rr = cat::simd_rsqrt(unit);
   cat::scalar_simd<float4> const rr_scalar = cat::simd_rsqrt(unit_ref);
   for (idx i = 0u; i < idx{4}; ++i) {
      cat::verify(cat::abs(rr[i] - 0.5_f4) < 0.001_f4);
      cat::verify(cat::abs(rr[i] - rr_scalar[i]) < 0.002_f4);
   }
}

$test(simd_sse2_abi_hooks_mask_to_bitset) {
   x64::sse2_simd<float> const v{1.f, 2.f, 3.f, 4.f};
   auto const m = v > x64::sse2_simd<float>{2.f, 2.f, 2.f, 2.f};
   cat::bitset<4u> const bits = m.to_bitset();
   cat::verify(bits[0u] == false);
   cat::verify(bits[1u] == false);
   cat::verify(bits[2u] == true);
   cat::verify(bits[3u] == true);
}

// AVX2 hooks (`simd_avx2.hpp`).
// `unary_full<op_rsqrt>`, plus mask reductions via `simd_avx2_mask_ops.hpp` and
// `simd_avx2_mask_ops.hpp` mask reductions. `mask_to_bitset` via
// `avx2_abi_mask_to_bitset`.
$test(simd_avx2_abi_hooks_permute_and_rsqrt) {
   x64::avx2_simd<float4> const fv{10_f4, 20_f4, 30_f4, 40_f4,
                                   50_f4, 60_f4, 70_f4, 80_f4};
   int4x8 const fid{0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u};
   cat::verify(cat::simd_permute(fv, fid) == fv);

   cat::scalar_simd<float4> const flo{10_f4, 20_f4, 30_f4, 40_f4};
   int4x4 const rev4{3u, 2u, 1u, 0u};
   int4x8 const rev_low{3u, 2u, 1u, 0u, 4u, 5u, 6u, 7u};
   auto const perm_avx = cat::simd_permute(fv, rev_low);
   cat::verify(cat::simd_permute(flo, rev4)
               == cat::scalar_simd<float4>(perm_avx[0u], perm_avx[1u],
                                           perm_avx[2u], perm_avx[3u]));

   int4x8 const fbroadcast{};
   cat::verify(cat::simd_permute(fv, fbroadcast)[7u] == fv[0u]);

   x64::avx2_simd<int4> const iv{1, 2, 3, 4, 5, 6, 7, 8};
   cat::verify(cat::simd_permute(iv, fid) == iv);
   cat::scalar_simd<int4> const ilo{1, 2, 3, 4};
   int4x4 const irev4{3u, 2u, 1u, 0u};
   int4x8 const irev_low{3u, 2u, 1u, 0u, 4u, 5u, 6u, 7u};
   auto const iperm = cat::simd_permute(iv, irev_low);
   cat::verify(
      cat::simd_permute(ilo, irev4)
      == cat::scalar_simd<int4>(iperm[0u], iperm[1u], iperm[2u], iperm[3u]));

   x64::avx2_simd<double> const dv{1., 2., 3., 4.};
   int4x4 const did{0u, 1u, 2u, 3u};
   cat::verify(cat::simd_permute(dv, did) == dv);

   int4x4 const drev{3u, 2u, 1u, 0u};
   auto const swapped = cat::simd_permute(dv, drev);
   cat::verify(swapped[0u] == 4.);
   cat::verify(swapped[3u] == 1.);
   cat::simd<double, cat::scalar_abi<double>> const pair_front{4., 3.};
   cat::simd<double, cat::scalar_abi<double>> const pair_back{2., 1.};
   cat::verify(swapped[0u] == pair_front[0u]);
   cat::verify(swapped[1u] == pair_front[1u]);
   cat::verify(swapped[2u] == pair_back[0u]);
   cat::verify(swapped[3u] == pair_back[1u]);

   x64::avx2_simd<float4> ones{};
   ones.fill(4_f4);
   cat::scalar_simd<float4> ones_ref{};
   ones_ref.fill(4_f4);
   auto const rr = cat::simd_rsqrt(ones);
   cat::scalar_simd<float4> const rr_scalar = cat::simd_rsqrt(ones_ref);
   for (idx i = 0u; i < idx{8}; ++i) {
      idx const j = idx{static_cast<uword>(i.raw % 4u)};
      cat::verify(cat::abs(rr[i] - 0.5_f4) < 0.001_f4);
      cat::verify(cat::abs(rr[i] - rr_scalar[j]) < 0.002_f4);
   }
}

$test(simd_avx2_rsqrt_times_sqrt_near_one) {
   using avxf = cat::simd<float, x64::avx2_abi<float>>;
   using sxf = cat::simd<float, cat::scalar_abi<float>>;
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

$test(simd_avx2_abi_hooks_mask_reductions_and_bitset) {
   x64::avx2_simd_mask<float4> const sparse{false, false, true,  false,
                                            false, true,  false, true};
   // Eight `float4` lanes need 32 bytes. `scalar_abi` is fixed at 16 bytes
   // (four lanes). Compare against `fixed_size_abi<float4, 8>` instead of
   // `native_abi<float4>` (which aliases `avx2_abi<float4>` here). No AVX2 mask
   // hook specializations apply to `fixed_size_abi`.
   cat::simd_mask<float4, cat::fixed_size_abi<float4, 8>> const sparse_ref{
      false, false, true, false, false, true, false, true};
   cat::verify(sparse.count_if_true() == sparse_ref.count_if_true());
   cat::verify(sparse.find_if_true() == sparse_ref.find_if_true());
   cat::verify(sparse.find_last_if_true() == sparse_ref.find_last_if_true());

   x64::avx2_simd_mask<float4> const all_true{true, true, true, true,
                                              true, true, true, true};
   cat::simd_mask<float4, cat::fixed_size_abi<float4, 8>> const all_true_ref{
      true, true, true, true, true, true, true, true};
   cat::verify(all_true.count_if_true() == all_true_ref.count_if_true());
   cat::verify(cat::simd_all_of(all_true) == cat::simd_all_of(all_true_ref));
   cat::verify(all_true.find_if_true() == all_true_ref.find_if_true());
   cat::verify(all_true.find_last_if_true()
               == all_true_ref.find_last_if_true());

   x64::avx2_simd_mask<float4> const none{false, false, false, false,
                                          false, false, false, false};
   cat::simd_mask<float4, cat::fixed_size_abi<float4, 8>> const none_ref{
      false, false, false, false, false, false, false, false};
   cat::verify(none.count_if_true() == none_ref.count_if_true());
   cat::verify(cat::simd_any_of(none) == cat::simd_any_of(none_ref));

   x64::avx2_simd_mask<float4> const one{false, false, false, true,
                                         false, false, false, false};
   cat::simd_mask<float4, cat::fixed_size_abi<float4, 8>> const one_ref{
      false, false, false, true, false, false, false, false};
   cat::verify(one.find_if_true() == one_ref.find_if_true());
   cat::verify(one.find_last_if_true() == one_ref.find_last_if_true());

   cat::simd_mask<float4, cat::scalar_abi<float4>> const sparse_lo{
      sparse[0u], sparse[1u], sparse[2u], sparse[3u]};
   cat::simd_mask<float4, cat::scalar_abi<float4>> const sparse_hi{
      sparse[4u], sparse[5u], sparse[6u], sparse[7u]};
   cat::verify(sparse.count_if_true()
               == sparse_lo.count_if_true() + sparse_hi.count_if_true());

   cat::bitset<8u> const b = sparse.to_bitset();
   cat::bitset<8u> const b_ref = sparse_ref.to_bitset();
   for (idx k = 0u; k < 8u; ++k) {
      cat::verify(static_cast<bool>(b[k]) == static_cast<bool>(b_ref[k]));
   }
   for (idx i = 0u; i < 8u; ++i) {
      cat::verify(b[i] == sparse[i]);
   }
}

$test(simd_avx2_unaligned_abi) {
   static_assert(x64::avx2_unaligned_simd<float>::size() == 8u);
   static_assert(alignof(x64::avx2_unaligned_simd<float>) == 1);

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
      allocator.unalign_alloc<x64::avx2_unaligned_simd<float>>().verify();

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

$test(simd_sse2_and_unaligned_abi) {
   static_assert(x64::sse2_unaligned_simd<float>::size() == 4u);
   static_assert(alignof(x64::sse2_unaligned_simd<float>) == 1);

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
      allocator.unalign_alloc<x64::sse2_unaligned_simd<float>>().verify();

   cat::verify(!cat::is_aligned(p_unaligned, 16_uz));
   cat::verify(cat::is_aligned(p_unaligned, 2_uz));

   cat::verify(!cat::is_aligned(p_unaligned->data(), 16_uz));
   cat::verify(cat::is_aligned(p_unaligned->data(), 2_uz));

   p_unaligned->fill(4.f);

   cat::verify(p_unaligned[0u] == 4.f);

   auto const rr = cat::simd_rsqrt(*p_unaligned);
   cat::verify(cat::abs(rr[0u] - 0.5_f4) < 0.001_f4);
}

$test(simd_avx2_unaligned_abi_mask) {
   static_assert(x64::avx2_unaligned_simd_mask<float>::size() == 8u);
   static_assert(alignof(x64::avx2_unaligned_simd_mask<float>) == 1);

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
      allocator.unalign_alloc<x64::avx2_unaligned_simd_mask<float>>().verify();

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

$test(simd_sse2_and_unaligned_abi_mask) {
   static_assert(x64::sse2_unaligned_simd_mask<float>::size() == 4u);
   static_assert(alignof(x64::sse2_unaligned_simd_mask<float>) == 1);

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
      allocator.unalign_alloc<x64::sse2_unaligned_simd_mask<float>>().verify();

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
   static_assert(cat::is_same<x64::avx2_unaligned_abi<float>,
                              cat::unaligned_abi<x64::avx2_abi<float>>>);
   static_assert(cat::is_same<x64::sse2_unaligned_abi<float>,
                              cat::unaligned_abi<x64::sse2_abi<float>>>);

   static_assert(cat::simd_abi::unaligned<cat::native_abi<float>>::lanes
                 == cat::native_abi<float>::lanes);
   static_assert(cat::simd_abi::unaligned<cat::native_abi<float>>::size
                 == cat::native_abi<float>::size);
   static_assert(cat::simd_abi::unaligned<cat::native_abi<float>>::alignment
                 == 1u);

   static_assert(alignof(cat::native_unaligned_simd<float>) == 1u);
   static_assert(alignof(cat::fixed_size_unaligned_simd<cat::float4, 4u>)
                 == 1u);
   static_assert(alignof(cat::scalar_unaligned_simd<cat::float4>) == 1u);
   static_assert(alignof(cat::compatible_unaligned_simd<float>) == 1u);
}
