#include <cat/array>
#include <cat/linear_allocator>
#include <cat/math>
#include <cat/page_allocator>
#include <cat/simd>
#include <cat/simd_iterator>
#include <cat/simd_ops>
#include <cat/vec>

#include "../unit_tests.hpp"

using cat::compatible_abi;
using cat::fixed_size_abi;
using cat::fixed_size_simd;
using cat::float4;
using cat::float4x4;
using cat::float4x8;
using cat::float8;
using cat::float8x2;
using cat::int1;
using cat::int2;
using cat::int4;
using cat::int4x4;
using cat::int4x8;
using cat::int8;
using cat::is_simd_array_like;
using cat::native_abi;
using cat::scalar_abi;
using cat::uint1;
using cat::uint4;
using cat::uint4x4;
using cat::unaligned_abi;

using namespace cat::literals;

namespace {
using int_lane = cat::int4::raw_type;
using uint_lane = cat::uint4::raw_type;
using float_lane = cat::float4::raw_type;
using double_lane = cat::float8::raw_type;
using mask_lane = cat::uint1::raw_type;

template <typename WideBoolSimd>
void
verify_widened_bool_simd_matches_bool4(cat::fixed_size_simd<bool, 4u> const ref,
                                       WideBoolSimd const v) {
   cat::verify(ref.size() == v.size());
   for (cat::idx i = 0u; i < 4u; ++i) {
      cat::verify(v[i] == ref[i]);
   }
}

}  // namespace

test(simd) {
   int4x4 vec1 = {0, 1, 2, 3};
   int4x4 vec2{0, 1, 2, 3};
   vec1 += vec2;
   int4x4 const sum = vec1 + vec2;
   cat::verify(sum == int4x4{0, 3, 6, 9});

   int4x4 const same = {5, 6, 7, 8};
   int4x4 const twin = {5, 6, 7, 8};
   int4x4 const diff_last = {5, 6, 7, 9};
   cat::verify(same == twin);
   cat::verify(!(same == diff_last));
   int4x4 const all_five(5);
   cat::verify(all_five == 5);
   cat::verify(5 == all_five);
   auto const lane_eq = cat::simd_equal_lanes(same, diff_last);
   cat::verify(lane_eq[0] && lane_eq[1] && lane_eq[2] && !lane_eq[3]);
   cat::verify(same.equal_lanes(diff_last)[0] && same.equal_lanes(diff_last)[1]
               && same.equal_lanes(diff_last)[2]
               && !same.equal_lanes(diff_last)[3]);
   cat::verify(cat::simd_unequal_lanes(same, diff_last)[3]);
   cat::verify(same.unequal_lanes(twin).none_of());
   cat::verify(same.equal_lanes(twin).all_of());

   int4x4 const iota_vec = {0, 1, 2, 3};
   idx range_i = 0u;
   for (auto lane : iota_vec) {
      cat::verify(lane == range_i);
      ++range_i;
   }
   cat::verify(range_i == iota_vec.size());
   cat::verify(iota_vec.begin() != iota_vec.end());
   {
      auto it = iota_vec.begin();
      for (idx k = 0u; k < iota_vec.size(); ++k) {
         cat::verify(*it == iota_vec[k]);
         ++it;
      }
      cat::verify(it == iota_vec.end());
   }
   {
      int4x4 lane_it{};
      auto p = lane_it.begin();
      auto q = lane_it.begin();
      cat::verify(iword(p - q) == iword(0));
      ++p;
      cat::verify(iword(p - q) == iword(1));
      cat::verify((p <=> q) > 0);
   }

   float4x4 f1 = {0_f4, 1_f4, 2_f4, 3_f4};
   float4x4 f2{0_f4, 1_f4, 2_f4, 3_f4};
   f1 += f2;
   auto fsum = f1 + f2;
   auto _ = fsum;

   float4x4 const broadcast_from_int(1);
   cat::verify(broadcast_from_int == 1_f4);
}

test(simd_broadcast_traits) {
   using cat::detail::has_simd_broadcast_consteval_value;
   using cat::detail::simd_broadcast_really_convertible_to;
   using cat::detail::simd_consteval_broadcast_arg;

   static_assert(simd_broadcast_really_convertible_to<int, int4>());
   static_assert(simd_broadcast_really_convertible_to<unsigned int, uint4>());
   static_assert(simd_broadcast_really_convertible_to<float, float4>());
   static_assert(!simd_broadcast_really_convertible_to<int, float4>());

   static_assert(simd_consteval_broadcast_arg<int, float4>);
   static_assert(simd_consteval_broadcast_arg<unsigned int, float4>);
   static_assert(!simd_consteval_broadcast_arg<int, int4>);
   static_assert(!simd_consteval_broadcast_arg<unsigned int, uint4>);
   static_assert(!simd_consteval_broadcast_arg<float, float4>);

   cat::verify(has_simd_broadcast_consteval_value<int, float4>(0));
   cat::verify(has_simd_broadcast_consteval_value<int, float4>(42));
   cat::verify(has_simd_broadcast_consteval_value<unsigned int, float4>(7u));

   int4x4 const from_int_lane(123);
   cat::verify(from_int_lane == int4x4{123, 123, 123, 123});

   float4x4 const from_int_constant(9);
   cat::verify(from_int_constant == 9_f4);

   float4x4 const from_uint_constant(4u);
   cat::verify(from_uint_constant == 4_f4);
}

test(simd_lane_iterator) {
   using Simd = int4x4;
   using lane_iter = Simd::iterator;
   static_assert(cat::is_same<typename lane_iter::iterator_concept,
                              cat::random_access_iterator_tag>);
   static_assert(cat::is_same<typename lane_iter::reference,
                              typename lane_iter::value_type>);
   static_assert(
      cat::is_same<typename lane_iter::pointer,
                   cat::proxy_arrow_result<typename lane_iter::value_type>>);

   Simd v = {10, 20, 30, 40};
   auto b = v.begin();
   cat::verify(b.lane_index() == 0u);
   cat::verify(*b == v[0u]);

   Simd const& cv = v;
   cat::verify(*cv.begin() == v[0u]);

   auto p_post = b++;
   cat::verify(p_post.lane_index() == 0u);
   cat::verify(b.lane_index() == 1u);
   ++b;
   cat::verify(b.lane_index() == 2u);
   b += iword(-1);
   cat::verify(b.lane_index() == 1u);

   cat::verify(b[iword(1)] == v[2u]);

   auto end_it = v.begin() + iword(Simd::size());
   cat::verify(b != cat::default_sentinel);
   cat::verify(end_it == cat::default_sentinel);
   cat::verify(cat::default_sentinel == end_it);
   cat::verify(iword(end_it - cat::default_sentinel) == iword(0));
   cat::verify(iword(cat::default_sentinel - end_it) == iword(0));

   auto c = v.begin();
   cat::verify(iword(1) + c == c + iword(1));

   lane_iter it0 = v.begin();
   lane_iter it3 = it0 + iword(3);
   cat::verify(*it3 == v[3u]);
   cat::verify((it3 <=> it0) > 0);
   cat::verify(iword(it3 - it0) == iword(3));

   cat::verify(iword(cat::default_sentinel - v.begin()) == iword(4));

   auto tmp = v.begin();
   cat::verify(--(++tmp) == v.begin());

   auto tmp2 = v.begin() + iword(2);
   auto old = tmp2--;
   cat::verify(old.lane_index() == 2u);
   cat::verify(tmp2.lane_index() == 1u);
}

test(simd_size_data_and_mask_size_data) {
   using int_vec = cat::simd<int, cat::fixed_size_abi<int, 4u>>;
   int_vec vi = {10, 20, 30, 40};
   cat::verify(int_vec::size() == 4u);
   cat::verify(vi.size() == 4u);
   cat::verify(vi.data() != nullptr);
   vi.set_lane(0u, 99);
   cat::verify(vi[0u] == 99);

   using int_vec_mask = int_vec::mask_type;
   int_vec_mask mi;
   bool const pattern[] = {true, false, true, false};
   mi.load(pattern);
   cat::verify(int_vec_mask::size() == 4u);
   cat::verify(mi.size() == 4u);
   cat::verify(mi.data() != nullptr);
}

test(fixed_size_simd_alignment) {
   using cat::fixed_size_abi;
   using cat::fixed_size_simd;
   using cat::float4;
   using cat::float8;
   using cat::int4;
   using cat::uword;

   // `x64::avx2_abi` cap is 32 bytes. Alignment is the largest power of two not
   // exceeding `min(size, 32)`, at least `alignof(T)`.
   static_assert(fixed_size_abi<float4, 1u>::size == 4u);
   static_assert(fixed_size_abi<float4, 1u>::alignment == 4u);
   static_assert(fixed_size_abi<float4, 2u>::size == 8u);
   static_assert(fixed_size_abi<float4, 2u>::alignment == 8u);
   static_assert(fixed_size_abi<float4, 3u>::size == 12u);
   static_assert(fixed_size_abi<float4, 3u>::alignment == 8u);
   static_assert(fixed_size_abi<float4, 4u>::size == 16u);
   static_assert(fixed_size_abi<float4, 4u>::alignment == 16u);
   static_assert(fixed_size_abi<float4, 6u>::size == 24u);
   static_assert(fixed_size_abi<float4, 6u>::alignment == 16u);
   static_assert(fixed_size_abi<float4, 8u>::size == 32u);
   static_assert(fixed_size_abi<float4, 8u>::alignment == 32u);
   static_assert(fixed_size_abi<float4, 16u>::size == 64u);
   static_assert(fixed_size_abi<float4, 16u>::alignment == 32u);

   static_assert(fixed_size_abi<float8, 2u>::size == 16u);
   static_assert(fixed_size_abi<float8, 2u>::alignment == 16u);
   static_assert(fixed_size_abi<float8, 4u>::size == 32u);
   static_assert(fixed_size_abi<float8, 4u>::alignment == 32u);

   static_assert(fixed_size_abi<int4, 4u>::size == 16u);
   static_assert(fixed_size_abi<int4, 4u>::alignment == 16u);
   static_assert(fixed_size_abi<int4, 8u>::size == 32u);
   static_assert(fixed_size_abi<int4, 8u>::alignment == 32u);

   static_assert(alignof(fixed_size_simd<float4, 1u>) == 4u);
   static_assert(alignof(fixed_size_simd<float4, 2u>) == 8u);
   static_assert(alignof(fixed_size_simd<float4, 4u>) == 16u);
   static_assert(alignof(fixed_size_simd<float4, 8u>) == 32u);
   static_assert(alignof(fixed_size_simd<int4, 4u>) == 16u);
   static_assert(alignof(fixed_size_simd<int4, 8u>) == 32u);
   static_assert(alignof(fixed_size_simd<float8, 2u>) == 16u);
   static_assert(alignof(fixed_size_simd<float8, 4u>) == 32u);

   cat::verify(alignof(float4x4) == 16u);
   cat::verify(alignof(float4x8) == 32u);
   cat::verify(alignof(int4x4) == 16u);
   cat::verify(alignof(int4x8) == 32u);
}

test(simd_is_array_like_v) {
   // P3983R1 mandates array-like layout for `cat::native_abi` satisfies it for
   // every supported lane type today.
   static_assert(is_simd_array_like<int1, native_abi<int1>>);
   static_assert(is_simd_array_like<int2, native_abi<int2>>);
   static_assert(is_simd_array_like<int4, native_abi<int4>>);
   static_assert(is_simd_array_like<int8, native_abi<int8>>);
   static_assert(is_simd_array_like<uint1, native_abi<uint1>>);
   static_assert(is_simd_array_like<uint4, native_abi<uint4>>);
   static_assert(is_simd_array_like<float4, native_abi<float4>>);
   static_assert(is_simd_array_like<float8, native_abi<float8>>);
   static_assert(is_simd_array_like<bool, native_abi<bool>>);
   static_assert(is_simd_array_like<char, native_abi<char>>);

   // `compatible_abi` shares `native_abi` storage today.
   static_assert(is_simd_array_like<int4, compatible_abi<int4>>);
   static_assert(is_simd_array_like<float4, compatible_abi<float4>>);

   // `scalar_abi` is always 16 bytes, a power of two divisible by every
   // supported scalar size.
   static_assert(is_simd_array_like<int1, scalar_abi<int1>>);
   static_assert(is_simd_array_like<int4, scalar_abi<int4>>);
   static_assert(is_simd_array_like<int8, scalar_abi<int8>>);
   static_assert(is_simd_array_like<float4, scalar_abi<float4>>);

   // `fixed_size_abi` where lanes * sizeof(`T`) is itself a power of two has no
   // trailing padding.
   static_assert(is_simd_array_like<float4, fixed_size_abi<float4, 1u>>);
   static_assert(is_simd_array_like<float4, fixed_size_abi<float4, 2u>>);
   static_assert(is_simd_array_like<float4, fixed_size_abi<float4, 4u>>);
   static_assert(is_simd_array_like<float4, fixed_size_abi<float4, 8u>>);
   static_assert(is_simd_array_like<float8, fixed_size_abi<float8, 2u>>);
   static_assert(is_simd_array_like<float8, fixed_size_abi<float8, 4u>>);
   static_assert(is_simd_array_like<int4, fixed_size_abi<int4, 4u>>);
   static_assert(is_simd_array_like<int4, fixed_size_abi<int4, 8u>>);
   static_assert(is_simd_array_like<int8, fixed_size_abi<int8, 2u>>);
   static_assert(is_simd_array_like<int8, fixed_size_abi<int8, 4u>>);
   static_assert(is_simd_array_like<int1, fixed_size_abi<int1, 16u>>);
   static_assert(is_simd_array_like<int1, fixed_size_abi<int1, 32u>>);

   // `fixed_size_abi` where lanes * sizeof(`T`) is not a power of two: Clang
   // `gnu::vector_size` rounds the storage up which leaves trailing padding, so
   // the layout is not array-like. P3983R1 anticipates this for `fixed_size`.
   static_assert(!is_simd_array_like<float4, fixed_size_abi<float4, 3u>>);
   static_assert(!is_simd_array_like<float4, fixed_size_abi<float4, 5u>>);
   static_assert(!is_simd_array_like<float4, fixed_size_abi<float4, 6u>>);
   static_assert(!is_simd_array_like<float4, fixed_size_abi<float4, 7u>>);
   static_assert(!is_simd_array_like<int8, fixed_size_abi<int8, 3u>>);

   // The unaligned wrapper preserves the base ABI's storage size, so it
   // inherits its array-likeness.
   static_assert(is_simd_array_like<float4, unaligned_abi<native_abi<float4>>>);
   static_assert(
      is_simd_array_like<float4, unaligned_abi<fixed_size_abi<float4, 4u>>>);
   static_assert(
      !is_simd_array_like<float4, unaligned_abi<fixed_size_abi<float4, 3u>>>);

   // P3983R1 note: when the trait is `true`, `bit_cast` between `simd<T, Abi>`
   // and the corresponding `array<T, N>` is well-defined and preserves lane
   // order.
   using simd_4f = fixed_size_simd<float4, 4u>;
   using array_4f = cat::array<float4, 4u>;
   static_assert(is_simd_array_like<float4, simd_4f::abi_type>);
   static_assert(sizeof(simd_4f) == sizeof(array_4f));
   static_assert(cat::is_trivially_copyable<simd_4f>);
   simd_4f const v{1_f4, 2_f4, 3_f4, 4_f4};
   auto const lanes = __builtin_bit_cast(array_4f, v);
   cat::verify(lanes[0] == 1_f4);
   cat::verify(lanes[1] == 2_f4);
   cat::verify(lanes[2] == 3_f4);
   cat::verify(lanes[3] == 4_f4);
}

test(simd_abi_compatible_alias) {
   static_assert(!cat::is_same<cat::simd_abi::compatible<int>,
                               cat::simd_abi::native<int>>);
   cat::verify(cat::simd<int, cat::compatible_abi<int>>::abi_type::lanes
               == cat::simd<int, cat::native_abi<int>>::abi_type::lanes);
}

test(simdu) {
   using idx_abi = cat::native_abi<cat::idx>;
   using idxv = cat::simd<cat::idx, idx_abi>;
   static_assert(cat::is_simd<idxv>);
   constexpr idx lane_count = idx_abi::lanes;
   static_assert(lane_count.raw * sizeof(cat::idx) == idx_abi::size.raw);

   idxv const ramp = cat::iota<idxv>(0u);
   for (idx i = 0u; i < lane_count; ++i) {
      cat::verify(ramp[i] == i);
   }

   idxv const ten = cat::make_simd_filled<idxv>(10u);
   cat::verify(ten == 10u);

   idxv const sum = ramp + ten;
   for (idx i = 0u; i < lane_count; ++i) {
      cat::verify(sum[i] == i + 10u);
   }

   cat::verify(ramp.equal_lanes(ramp).all_of());
   cat::verify(cat::simd_equal_lanes(ramp, ramp).all_of());

   auto const m = cat::make_simd_mask_from_count<idxv>(2u);
   idxv const ones = cat::make_simd_filled<idxv>(1u);
   idxv const twos = cat::make_simd_filled<idxv>(2u);
   idxv const blended = cat::simd_if_else(m, ones, twos);
   cat::verify(blended[0u] == 1u);
   cat::verify(blended[1u] == 1u);
   if (lane_count > 2u) {
      cat::verify(blended[2u] == 2u);
   }

   idx seen = 0u;
   for (auto lane : ramp) {
      cat::verify(lane == seen);
      ++seen;
   }
   cat::verify(seen == lane_count);
}

test(simd_uintptr_intptr) {
   using u_ptr = cat::uintptr<void>;
   using u_abi = cat::native_abi<u_ptr>;
   using upv = cat::simd<u_ptr, u_abi>;
   static_assert(cat::is_simd<upv>);
   constexpr idx ulanes = u_abi::lanes;
   static_assert(ulanes.raw * sizeof(u_ptr) == u_abi::size.raw);

   upv const uramp = cat::iota<upv>(u_ptr{});
   for (idx i = 0u; i < ulanes; ++i) {
      cat::verify(uramp[i].raw == i.raw);
   }

   upv const usplat = cat::make_simd_filled<upv>(42_uz);
   cat::verify(usplat == u_ptr(42_uz));
   for (idx i = 0u; i < ulanes; ++i) {
      cat::verify((uramp + usplat)[i].raw == i.raw + 42_uz);
   }

   cat::verify(uramp.equal_lanes(uramp).all_of());

   using i_ptr = cat::intptr<void>;
   using i_abi = cat::native_abi<i_ptr>;
   using ipv = cat::simd<i_ptr, i_abi>;
   static_assert(cat::is_simd<ipv>);
   constexpr idx ilanes = i_abi::lanes;

   ipv const iramp = cat::iota<ipv>(i_ptr{});
   for (idx i = 0u; i < ilanes; ++i) {
      cat::verify(iramp[i].raw == static_cast<i_ptr::raw_type>(i.raw));
   }

   ipv const isplat = cat::make_simd_filled<ipv>(11);
   cat::verify(isplat == i_ptr(11));
   if (ilanes > 1u) {
      cat::verify((iramp + isplat)[1u].raw
                  == static_cast<i_ptr::raw_type>(1 + 11));
   }
}

test(simd_rebind_and_resize_traits) {
   using floatv = cat::simd<float, cat::native_abi<float>>;
   using doublev = cat::rebind_simd<double, floatv>;
   static_assert(
      cat::is_same<doublev, cat::simd<double, cat::native_abi<double>>>);

   using float4 = cat::fixed_size_simd<float, 4u>;
   using float2 = cat::resize_simd<2u, float4>;
   static_assert(cat::is_same<float2, cat::fixed_size_simd<float, 2u>>);

   using maskf = cat::simd_mask<float, cat::native_abi<float>>;
   using maskd = cat::rebind_simd<double, maskf>;
   static_assert(
      cat::is_same<maskd, cat::simd_mask<double, cat::native_abi<double>>>);
}

test(simd_resize_insert_extract) {
   int4x4 const v = {10, 11, 12, 13};

   auto const shr = cat::simd_resize<2u>(v);
   static_assert(decltype(shr)::abi_type::lanes == 2);
   cat::verify(shr[0] == 10);
   cat::verify(shr[1] == 11);

   auto const grow = cat::simd_resize<6u>(v);
   static_assert(decltype(grow)::abi_type::lanes == 6);
   cat::verify(grow[0] == 10);
   cat::verify(grow[1] == 11);
   cat::verify(grow[2] == 12);
   cat::verify(grow[3] == 13);
   cat::verify(grow[4].raw == 0);
   cat::verify(grow[5].raw == 0);

   auto const slice = cat::simd_extract<1u, 3u>(v);
   static_assert(decltype(slice)::abi_type::lanes == 2);
   cat::verify(slice[0] == 11);
   cat::verify(slice[1] == 12);

   using int4x2 = cat::fixed_size_simd<int4, 2u>;
   int4x2 const chunk = {40, 41};
   int4x4 const patched = cat::simd_insert<1u>(v, chunk);
   cat::verify(patched[0] == 10);
   cat::verify(patched[1] == 40);
   cat::verify(patched[2] == 41);
   cat::verify(patched[3] == 13);

   using M = int4x4::mask_type;
   M const mk = {true, false, true, false};
   auto const mk2 = cat::simd_resize<2u>(mk);
   cat::verify(mk2[0]);
   cat::verify(!mk2[1]);

   auto const mk_slice = cat::simd_extract<2u, 4u>(mk);
   cat::verify(mk_slice[0]);
   cat::verify(!mk_slice[1]);

   using pair_mask = cat::fixed_size_simd_mask<int4, 2u>;
   pair_mask const pair_true = {true, true};
   M const all_false = {false, false, false, false};
   M const blended = cat::simd_insert<1u>(all_false, pair_true);
   cat::verify(!blended[0] && blended[1] && blended[2] && !blended[3]);
}

test(simd_if_else_mask_from_count) {
   int4x4 a = {0, 1, 2, 3};
   int4x4 b = {10, 20, 30, 40};
   auto m = cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 r = cat::simd_if_else(m, a, b);
   cat::verify(r[0] == 0);
   cat::verify(r[1] == 1);
   cat::verify(r[2] == 30);
   cat::verify(r[3] == 40);

   float4x4 fa = {0_f4, 1_f4, 2_f4, 3_f4};
   float4x4 fb = {10_f4, 20_f4, 30_f4, 40_f4};
   auto mf = cat::make_simd_mask_from_count<float4x4>(2u);
   float4x4 fr = cat::simd_if_else(mf, fa, fb);
   cat::verify(fr[0] == 0_f4);
   cat::verify(fr[1] == 1_f4);
   cat::verify(fr[2] == 30_f4);
   cat::verify(fr[3] == 40_f4);
}

test(simd_masked_increment) {
   int4x4 v = {1, -2, 3, -4};
   auto m = v >= 0;
   int4x4 inc = {10, 10, 10, 10};
   int4x4 r = cat::simd_if_else(m, v + inc, v);
   cat::verify(r[0] == 11);
   cat::verify(r[1] == -2);
   cat::verify(r[2] == 13);
   cat::verify(r[3] == -4);

   float4x4 fv = {1_f4, -2_f4, 3_f4, -4_f4};
   auto fm = fv >= 0_f4;
   float4x4 finc = {10_f4, 10_f4, 10_f4, 10_f4};
   float4x4 fr = cat::simd_if_else(fm, fv + finc, fv);
   cat::verify(fr[0] == 11_f4);
   cat::verify(fr[1] == -2_f4);
   cat::verify(fr[2] == 13_f4);
   cat::verify(fr[3] == -4_f4);
}

test(simd_shuffle_shufflevector) {
   int4x4 v = {10, 20, 30, 40};
   int4x4 r = cat::simd_shuffle<3, 2, 1, 0>(v);
   cat::verify(r[0] == 40);
   cat::verify(r[3] == 10);

   float4x4 fv = {10_f4, 20_f4, 30_f4, 40_f4};
   float4x4 fr = cat::simd_shuffle<3, 2, 1, 0>(fv);
   cat::verify(fr[0] == 40_f4);
   cat::verify(fr[3] == 10_f4);
}

test(simd_mask_pattern_and_if_else_builder) {
   using M = int4x4::mask_type;
   M m{5u};
   cat::verify(m.to_uword() == 5_uz);
   int4x4 v = {1, 2, 3, 4};
   int4x4 r = cat::simd_if_else(cat::make_simd_mask_if(m).else_(0), v + 100);
   cat::verify(r[0] == 101);
   cat::verify(r[1] == 0);
   cat::verify(r[2] == 103);
   cat::verify(r[3] == 0);

   using float_mask = float4x4::mask_type;
   float_mask mf{5u};
   cat::verify(mf.to_uword() == 5_uz);
   float4x4 fv = {1_f4, 2_f4, 3_f4, 4_f4};
   float4x4 fr =
      cat::simd_if_else(cat::make_simd_mask_if(mf).else_(0_f4), fv + 100_f4);
   cat::verify(fr[0] == 101_f4);
   cat::verify(fr[1] == 0_f4);
   cat::verify(fr[2] == 103_f4);
   cat::verify(fr[3] == 0_f4);
}

test(simd_sqrt_all_lanes_non_negative) {
   float4x4 v = {1_f4, 4_f4, 9_f4, 16_f4};
   float4x4 r = cat::simd_sqrt(v);
   cat::verify(r[0] == 1_f4);
   cat::verify(r[1] == 2_f4);
   cat::verify(r[2] == 3_f4);
   cat::verify(r[3] == 4_f4);
}

test(simd_rsqrt_rcbrt_rnroot) {
   float4x4 v = {1_f4, 4_f4, 9_f4, 16_f4};
   float4x4 const rr = cat::simd_rsqrt(v);
   cat::verify(rr[0] == 1_f4);
   cat::verify(rr[1] == 0.5_f4);
   cat::verify(rr[2] == (1_f4 / 3_f4));
   cat::verify(rr[3] == 0.25_f4);
   float4x4 const prod = rr * cat::simd_sqrt(v);
   cat::verify(prod[0] == 1_f4 && prod[1] == 1_f4 && prod[2] == 1_f4
               && prod[3] == 1_f4);

   float4x4 cubes = {8_f4, 27_f4, 64_f4, 125_f4};
   float4x4 const cr = cat::simd_rcbrt(cubes);
   cat::verify(cr[0] * cat::simd_cbrt(cubes)[0] == 1_f4);
   cat::verify(cr[1] * cat::simd_cbrt(cubes)[1] == 1_f4);

   float4x4 fourths = {1_f4, 16_f4, 81_f4, 256_f4};
   float4x4 const nr4 = cat::simd_rnroot(fourths, 4);
   cat::verify(nr4[0] == 1_f4);
   cat::verify(nr4[1] == 0.5_f4);
   cat::verify(nr4[2] == (1_f4 / 3_f4));
   cat::verify(nr4[3] == 0.25_f4);
   cat::verify(cat::simd_rnroot(v, 2) == cat::simd_rsqrt(v));

   float4x4 const inv = cat::simd_rnroot(v, 1);
   cat::verify(inv[0] == 1_f4);
   cat::verify(inv[1] == 0.25_f4);

   float4x4 const id0 = cat::simd_rnroot(v, 0);
   cat::verify(id0 == v);
}

test(simd_as_vectorized_stepanov_iterator) {
   int_lane const data[] = {10, 20, 30, 40};
   auto it = cat::as_vectorized<int4, 4u>(data);
   int4x4 chunk = *it;
   cat::verify(chunk[0] == 10);
   cat::verify(chunk[3] == 40);

   float_lane const fdata[] = {1.5f, 2.5f, 3.5f, 4.5f};
   auto fit = cat::as_vectorized<float4, 4u>(fdata);
   float4x4 fchunk = *fit;
   cat::verify(fchunk[0] == 1.5f);
   cat::verify(fchunk[3] == 4.5f);
}

// Pointer + idx length: load, store, `partial_*`. Prefer EVE-style masked
// `cat::simd_load_*[mask](passthrough, p)`/`cat::simd_store_*[mask](v, p)` when
// only some lanes are defined (see tests below).

test(simd_eve_mask_subscript_load_store_aligned) {
   alignas(int4x4::abi_type::alignment.raw)
      int_lane const src[] = {11, 22, 33, 44};
   int4x4 const pass = {10, 20, 30, 40};
   auto const m = cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 const r = cat::simd_load_aligned[m](pass, src).verify();
   cat::verify(r[0] == 11 && r[1] == 22 && r[2] == 30 && r[3] == 40);

   int4x4 const z = cat::simd_load_aligned[m](0, src).verify();
   cat::verify(z[0] == 11 && z[1] == 22 && z[2] == 0 && z[3] == 0);

   alignas(int4x4::abi_type::alignment.raw) int_lane dst[] = {-1, -1, -1, -1};
   int4x4 const v = {100, 200, 300, 400};
   cat::simd_store_aligned[m](v, dst);
   cat::verify(dst[0] == 100 && dst[1] == 200 && dst[2] == -1 && dst[3] == -1);

   int4x4 const factory = cat::simd_if_else(
      m, cat::make_simd_loaded_aligned<int4x4>(src).verify(), pass);
   cat::verify(r[0] == factory[0] && r[1] == factory[1] && r[2] == factory[2]
               && r[3] == factory[3]);

   alignas(float4x4::abi_type::alignment.raw)
      float_lane const fsrc[] = {1.1f, 2.2f, 3.3f, 4.4f};
   float4x4 const fpass = {10_f4, 20_f4, 30_f4, 40_f4};
   auto const mf = cat::make_simd_mask_from_count<float4x4>(2u);
   float4x4 const fr = cat::simd_load_aligned[mf](fpass, fsrc).verify();
   cat::verify(fr[0] == 1.1f && fr[1] == 2.2f && fr[2] == 30_f4
               && fr[3] == 40_f4);
   float4x4 const fz = cat::simd_load_aligned[mf](0_f4, fsrc).verify();
   cat::verify(fz[0] == 1.1f && fz[1] == 2.2f && fz[2] == 0_f4
               && fz[3] == 0_f4);
   alignas(float4x4::abi_type::alignment.raw)
      float_lane fdst[] = {-1.f, -1.f, -1.f, -1.f};
   float4x4 const fv = {100_f4, 200_f4, 300_f4, 400_f4};
   cat::simd_store_aligned[mf](fv, fdst);
   cat::verify(fdst[0] == 100_f4 && fdst[1] == 200_f4 && fdst[2] == -1.f
               && fdst[3] == -1.f);
   float4x4 const ffactory = cat::simd_if_else(
      mf, cat::make_simd_loaded_aligned<float4x4>(fsrc).verify(), fpass);
   cat::verify(fr[0] == ffactory[0] && fr[3] == ffactory[3]);
}

test(simd_eve_mask_subscript_load_store_unaligned_and_dispatch) {
   int_lane const src[] = {5, 6, 7, 8};
   int4x4 const pass = {99, 99, 99, 99};
   auto const m = cat::make_simd_mask_from_count<int4x4>(3u);
   int4x4 const r = cat::simd_load_unaligned[m](pass, src);
   cat::verify(r[0] == 5 && r[1] == 6 && r[2] == 7 && r[3] == 99);

   int4x4 const s = cat::simd_load[m](pass, src);
   cat::verify(s[0] == 5 && s[1] == 6 && s[2] == 7 && s[3] == 99);

   int_lane dst[4] = {0, 0, 0, 0};
   int4x4 const w = {1, 2, 3, 4};
   cat::simd_store_unaligned[cat::make_simd_mask_from_count<int4x4>(2u)](w,
                                                                         dst);
   cat::verify(dst[0] == 1 && dst[1] == 2 && dst[2] == 0 && dst[3] == 0);

   int_lane dst2[4] = {7, 7, 7, 7};
   auto const sm = cat::make_simd_mask_from_count<int4x4>(1u);
   cat::simd_store[sm](w, dst2);
   cat::verify(dst2[0] == 1 && dst2[1] == 7 && dst2[2] == 7 && dst2[3] == 7);

   float_lane const fsrc[] = {1.5f, 2.5f, 3.5f, 4.5f};
   float4x4 const fpass = {99_f4, 99_f4, 99_f4, 99_f4};
   auto const mf = cat::make_simd_mask_from_count<float4x4>(3u);
   float4x4 const fr = cat::simd_load_unaligned[mf](fpass, fsrc);
   cat::verify(fr[0] == 1.5f && fr[1] == 2.5f && fr[2] == 3.5f
               && fr[3] == 99_f4);
   float4x4 const fs = cat::simd_load[mf](fpass, fsrc);
   cat::verify(fs[0] == 1.5f && fs[3] == 99_f4);
   float_lane fdst[4] = {0.f, 0.f, 0.f, 0.f};
   float4x4 const fw = {1_f4, 2_f4, 3_f4, 4_f4};
   cat::simd_store_unaligned[cat::make_simd_mask_from_count<float4x4>(2u)](
      fw, fdst);
   cat::verify(fdst[0] == 1_f4 && fdst[1] == 2_f4 && fdst[2] == 0.f
               && fdst[3] == 0.f);
   float_lane fdst2[4] = {7.f, 7.f, 7.f, 7.f};
   auto const fsmm = cat::make_simd_mask_from_count<float4x4>(1u);
   cat::simd_store[fsmm](fw, fdst2);
   cat::verify(fdst2[0] == 1_f4 && fdst2[1] == 7.f && fdst2[2] == 7.f
               && fdst2[3] == 7.f);
}

test(simd_load_aligned_and_loaded_aligned) {
   alignas(int4x4::abi_type::alignment.raw)
      int_lane const src[] = {11, 22, 33, 44};
   int4x4 a{};
   a.load_aligned(src);
   cat::verify(a[0] == 11);
   cat::verify(a[3] == 44);
   int4x4 const b = cat::make_simd_loaded_aligned<int4x4>(src).verify();
   cat::verify(b[1] == 22);

   alignas(float4x4::abi_type::alignment.raw)
      float_lane const fsrc[] = {1.25f, 2.25f, 3.25f, 4.25f};
   float4x4 fa{};
   fa.load_aligned(fsrc);
   cat::verify(fa[0] == 1.25f);
   float4x4 const fb = cat::make_simd_loaded_aligned<float4x4>(fsrc).verify();
   cat::verify(fb[3] == 4.25f);
}

test(simd_aligned_load_returns_nullopt_when_misaligned) {
   alignas(int4x4::abi_type::alignment.raw)
      int_lane block[8] = {1, 2, 3, 4, 5, 6, 7, 8};
   int_lane const* const mis = block + 1;
   cat::verify(
      !cat::is_aligned(cat::unconst(mis), int4x4::abi_type::alignment));
   auto const full = cat::make_simd_loaded_aligned<int4x4>(mis);
   cat::verify(!full.has_value());

   int4x4 const pass = {10, 20, 30, 40};
   auto const m = cat::make_simd_mask_from_count<int4x4>(2u);
   auto const masked_make = cat::make_simd_loaded_aligned<int4x4>[m](pass, mis);
   cat::verify(!masked_make.has_value());
   auto const masked_load = cat::simd_load_aligned[m](pass, mis);
   cat::verify(!masked_load.has_value());
}

test(simd_load_unaligned_and_loaded_unaligned) {
   int_lane const src[] = {5, 6, 7, 8};
   int4x4 a{};
   a.load_unaligned(src);
   cat::verify(a[2] == 7);
   int4x4 b = cat::make_simd_loaded_unaligned<int4x4>(src);
   cat::verify(b[0] == 5);

   float_lane const fsrc[] = {5.f, 6.f, 7.f, 8.f};
   float4x4 fa{};
   fa.load_unaligned(fsrc);
   cat::verify(fa[2] == 7.f);
   float4x4 fb = cat::make_simd_loaded_unaligned<float4x4>(fsrc);
   cat::verify(fb[0] == 5.f);
}

test(simd_load_and_loaded_dispatch) {
   int_lane const src[] = {1, 2, 3, 4};
   int4x4 a{};
   a.load(src);
   cat::verify(a[1] == 2);
   int4x4 b = cat::make_simd_loaded<int4x4>(src);
   cat::verify(b[3] == 4);

   float_lane const fsrc[] = {1.f, 2.f, 3.f, 4.f};
   float4x4 fa{};
   fa.load(fsrc);
   cat::verify(fa[1] == 2.f);
   float4x4 fb = cat::make_simd_loaded<float4x4>(fsrc);
   cat::verify(fb[3] == 4.f);
}

// Byte lanes must use unaligned memory through `.load()`/`.store()` because
// arbitrary `char const*` is not guaranteed to match vector register alignment.
test(simd_byte_lane_load_store_dispatch_misaligned) {
   alignas(128) char backing[128]{};
   char* const p_mis = backing + 7;
   idx const lanes = cat::char1x_::abi_type::lanes;
   for (idx i = 0u; i < lanes; ++i) {
      p_mis[i] = static_cast<char>('A' + (i % 26));
   }

   cat::char1x_ loaded{};
   loaded.load(p_mis);
   idx const last_idx = lanes.raw - 1u;
   cat::verify(loaded[0u] == p_mis[0]);
   cat::verify(loaded[last_idx] == p_mis[last_idx.raw]);

   cat::char1x_ const from_loaded = cat::make_simd_loaded<cat::char1x_>(p_mis);
   cat::verify(from_loaded.equal_lanes(loaded).all_of());

   cat::native_unaligned_simd<char> loose{};
   loose.load(p_mis);
   cat::verify(loose[0u] == p_mis[0]);
   cat::verify(loose[last_idx] == p_mis[last_idx.raw]);

   alignas(128) char scratch[128]{};
   char* const p_mis_out = scratch + 11;
   loaded.store(p_mis_out);
   for (idx i = 0u; i < lanes; ++i) {
      cat::verify(p_mis_out[i] == p_mis[i]);
   }
}

test(simd_store_aligned_unaligned_and_dispatch) {
   alignas(int4x4::abi_type::alignment.raw) int_lane buf_a[4] = {};
   int4x4 v = {100, 200, 300, 400};
   v.store_aligned(buf_a);
   cat::verify(buf_a[0] == 100);
   cat::verify(buf_a[3] == 400);

   int_lane buf_u[4] = {};
   v.store_unaligned(buf_u);
   cat::verify(buf_u[2] == 300);

   int_lane buf_d[4] = {};
   v.store(buf_d);
   cat::verify(buf_d[1] == 200);

   alignas(float4x4::abi_type::alignment.raw) float_lane fbuf_a[4] = {};
   float4x4 fv = {1.5f, 2.5f, 3.5f, 4.5f};
   fv.store_aligned(fbuf_a);
   cat::verify(fbuf_a[0] == 1.5f);
   float_lane fbuf_u[4] = {};
   fv.store_unaligned(fbuf_u);
   cat::verify(fbuf_u[2] == 3.5f);
   float_lane fbuf_d[4] = {};
   fv.store(fbuf_d);
   cat::verify(fbuf_d[3] == 4.5f);
}

test(simd_partial_load_and_partial_loaded) {
   int_lane const short_src[] = {9, 8};
   int4x4 a = cat::make_simd_filled<int4x4>(99);
   a.partial_load(short_src, 2u);
   cat::verify(a[0] == 9);
   cat::verify(a[1] == 8);
   cat::verify(a[2] == 0);
   cat::verify(a[3] == 0);

   int4x4 b = cat::make_simd_partial_loaded<int4x4>(short_src, 2u);
   cat::verify(b[0] == 9 && b[1] == 8 && b[2] == 0 && b[3] == 0);

   int4x4 c = cat::make_simd_partial_loaded<int4x4>(short_src, 0u);
   cat::verify(c[0] == 0 && c[3] == 0);

   int_lane const four[] = {1, 2, 3, 4};
   int4x4 d = cat::make_simd_partial_loaded<int4x4>(four, 100u);
   cat::verify(d[0] == 1 && d[3] == 4);

   int4x4 e{};
   cat::simd_partial_load(e, short_src, 1u);
   cat::verify(e[0] == 9 && e[1] == 0);

   float_lane const fshort[] = {9.f, 8.f};
   float4x4 fp = cat::make_simd_filled<float4x4>(99.f);
   fp.partial_load(fshort, 2u);
   cat::verify(fp[0] == 9.f && fp[1] == 8.f && fp[2] == 0.f && fp[3] == 0.f);
   float4x4 fq = cat::make_simd_partial_loaded<float4x4>(fshort, 2u);
   cat::verify(fq[0] == 9.f && fq[3] == 0.f);
   float_lane const ffour[] = {1.f, 2.f, 3.f, 4.f};
   float4x4 fu = cat::make_simd_partial_loaded<float4x4>(ffour, 100u);
   cat::verify(fu[0] == 1.f && fu[3] == 4.f);

   float4x4 fe{};
   cat::simd_partial_load(fe, fshort, 1u);
   cat::verify(fe[0] == 9.f && fe[1] == 0.f);
}

test(simd_partial_store) {
   int4x4 v = {3, 4, 5, 6};
   int_lane out[4] = {-1, -1, -1, -1};
   v.partial_store(out, 2u);
   cat::verify(out[0] == 3);
   cat::verify(out[1] == 4);
   cat::verify(out[2] == -1);
   cat::verify(out[3] == -1);

   v.partial_store(out, 100u);
   cat::verify(out[0] == 3 && out[1] == 4 && out[2] == 5 && out[3] == 6);

   int_lane out2[4] = {};
   cat::simd_partial_store(v, out2, 3u);
   cat::verify(out2[0] == 3 && out2[1] == 4 && out2[2] == 5 && out2[3] == 0);

   float4x4 fv = {3.f, 4.f, 5.f, 6.f};
   float_lane fout[4] = {-1.f, -1.f, -1.f, -1.f};
   fv.partial_store(fout, 2u);
   cat::verify(fout[0] == 3.f && fout[1] == 4.f && fout[2] == -1.f);
   float_lane fout2[4] = {};
   cat::simd_partial_store(fv, fout2, 3u);
   cat::verify(fout2[0] == 3.f && fout2[3] == 0.f);
}

test(simd_load_unaligned_store_unaligned) {
   int_lane const src[] = {-5, -4, -3, -2};
   int4x4 a{};
   a.load_unaligned(src);
   cat::verify(a[2] == -3);
   int4x4 b = cat::make_simd_loaded_unaligned<int4x4>(src);
   cat::verify(b[0] == -5);

   int_lane buf[4] = {};
   b.store_unaligned(buf);
   cat::verify(buf[1] == -4);

   int_lane buf2[4] = {};
   b.store_unaligned(buf2);
   cat::verify(buf2[3] == -2);

   float_lane const fsrc[] = {-5.f, -4.f, -3.f, -2.f};
   float4x4 fa{};
   fa.load_unaligned(fsrc);
   cat::verify(fa[2] == -3.f);
   float4x4 fb = cat::make_simd_loaded_unaligned<float4x4>(fsrc);
   cat::verify(fb[0] == -5.f);
   float_lane fbuf[4] = {};
   fb.store_unaligned(fbuf);
   cat::verify(fbuf[1] == -4.f);
   float_lane fbuf2[4] = {};
   fb.store_unaligned(fbuf2);
   cat::verify(fbuf2[3] == -2.f);
}

test(simd_float_roundtrip) {
   float_lane const src[] = {1.5f, 2.5f, 3.5f, 4.5f};
   float4x4 v = cat::make_simd_loaded_unaligned<float4x4>(src);
   float_lane dst[4] = {};
   v.store_unaligned(dst);
   cat::verify(dst[0] == 1.5f);
   cat::verify(dst[3] == 4.5f);

   float4x4 p = cat::make_simd_partial_loaded<float4x4>(src, 2u);
   cat::verify(p[0] == 1.5f);
   cat::verify(p[1] == 2.5f);
   cat::verify(p[2] == 0.f);
}

// ane-wise arithmetic and bitwise (`cat::simd`):

test(simd_int_binary_ops) {
   int4x4 a = {10, 20, -5, 7};
   int4x4 b = {3, 4, 2, 2};
   int4x4 s = a + b;
   cat::verify(s[0] == 13);
   cat::verify(s[1] == 24);
   cat::verify(s[2] == -3);
   cat::verify(s[3] == 9);

   int4x4 d = a - b;
   cat::verify(d[0] == 7);
   cat::verify(d[2] == -7);

   int4x4 p = a * b;
   cat::verify(p[0] == 30);
   cat::verify(p[1] == 80);

   int4x4 q = a / b;
   cat::verify(q[0] == 3);
   cat::verify(q[1] == 5);

   float4x4 fa = {12_f4, 20_f4, -6_f4, 8_f4};
   float4x4 fb = {3_f4, 4_f4, 2_f4, 2_f4};
   float4x4 fs = fa + fb;
   cat::verify(fs[0] == 15_f4);
   cat::verify(fs[3] == 10_f4);
   float4x4 fd = fa - fb;
   cat::verify(fd[0] == 9_f4);
   float4x4 fp = fa * fb;
   cat::verify(fp[1] == 80_f4);
   float4x4 fq = fa / fb;
   cat::verify(fq[0] == 4_f4);
   cat::verify(fq[1] == 5_f4);
   cat::verify(fq[2] == -3_f4);
   cat::verify(fq[3] == 4_f4);

   int4x4 m = int4x4{7, 11, -9, 15} % int4x4{3, 5, 4, 8};
   cat::verify(m[0] == 1);
   cat::verify(m[1] == 1);
   cat::verify(m[2] == -1);
   cat::verify(m[3] == 7);
}

test(simd_scalar_on_left_ops) {
   int4x4 const v = {2, 4, 10, -5};
   int_lane const hundred = 100;

   int4x4 const sum_left = hundred + v;
   cat::verify(sum_left == int4x4{102, 104, 110, 95});
   cat::verify(sum_left == v + hundred);

   int4x4 const diff_left = hundred - v;
   cat::verify(diff_left == int4x4{98, 96, 90, 105});

   int_lane const three = 3;
   int4x4 const prod_left = three * v;
   cat::verify(prod_left == v * three);
   cat::verify(prod_left == int4x4{6, 12, 30, -15});

   int_lane const twenty_four = 24;
   int4x4 const quot_left = twenty_four / v;
   cat::verify(quot_left[0] == 24 / 2);
   cat::verify(quot_left[1] == 24 / 4);
   cat::verify(quot_left[2] == 24 / 10);
   cat::verify(quot_left[3] == 24 / -5);

   float4x4 const fv = {2_f4, 4_f4, 10_f4, 5_f4};
   float_lane const ten_f = 10.f;
   float4x4 const fsum_left = ten_f + fv;
   cat::verify(fsum_left == fv + ten_f);
   cat::verify(fsum_left[0] == 12_f4);

   float4x4 const fdiff_left = ten_f - fv;
   cat::verify(fdiff_left[0] == 8_f4);

   float_lane const three_f = 3.f;
   cat::verify(three_f * fv == fv * three_f);

   float_lane const hundred_f = 100.f;
   float4x4 const fquot_left = hundred_f / fv;
   cat::verify(fquot_left[0] == 50_f4);
}

test(simd_int_compound_assign) {
   int4x4 v = {1, 2, 3, 4};
   v += int4x4{10, 10, 10, 10};
   cat::verify(v[0] == 11);
   v -= int4x4{1, 1, 1, 1};
   cat::verify(v[1] == 11);
   v *= int4x4{2, 2, 2, 2};
   cat::verify(v[2] == 24);
   v /= int4x4{2, 2, 2, 2};
   cat::verify(v[3] == 13);
   v %= int4x4{3, 3, 3, 3};
   cat::verify(v[0] == 1);

   float4x4 fv = {1_f4, 2_f4, 3_f4, 4_f4};
   fv += float4x4{10_f4, 10_f4, 10_f4, 10_f4};
   cat::verify(fv[0] == 11_f4);
   fv -= float4x4{1_f4, 1_f4, 1_f4, 1_f4};
   cat::verify(fv[1] == 11_f4);
   fv *= float4x4{2_f4, 2_f4, 2_f4, 2_f4};
   cat::verify(fv[2] == 24_f4);
   fv /= float4x4{2_f4, 2_f4, 2_f4, 2_f4};
   cat::verify(fv[3] == 13_f4);
}

// Integral lanes only: bitwise and shift operators are not used on `float`
// SIMD.
test(simd_int_bitwise_and_shifts) {
   int4x4 a = {0x0f0f0f0f, 0x00ff00ff, -1, 0};
   int4x4 b = {0x00ff00ff, 0x0f0f0f0f, 0x12345678, 0x55};
   int4x4 x = a & b;
   cat::verify(x[0] == 0x000f000f);
   int4x4 o = a | b;
   cat::verify(o[0] == 0x0fff0fff);
   int4x4 xr = a ^ b;
   cat::verify(xr[0] == 0x0ff00ff0);

   int4x4 u = int4x4{1, 2, 4, 8};
   u &= int4x4{3, 3, 3, 3};
   cat::verify(u[0] == 1);
   u |= int4x4{8, 8, 8, 8};
   cat::verify(u[1] == 10);
   u ^= int4x4{15, 15, 15, 15};
   cat::verify(u[2] == 7);

   int4x4 const one = 1;
   int4x4 sh = int4x4{1, 2, 4, 8} << one;
   cat::verify(sh[0] == 2);
   int4x4 shr = int4x4{8, 16, 32, 64} >> one;
   cat::verify(shr[0] == 4);

   int4x4 y = int4x4{1, 2, 4, 8};
   y <<= one;
   cat::verify(y[1] == 4);
   y >>= one;
   cat::verify(y[2] == 4);
}

test(simd_float_ops) {
   float4x4 a = {10_f4, 20_f4, 5_f4, 8_f4};
   float4x4 b = {2_f4, 4_f4, 2_f4, 2_f4};
   float4x4 s = a + b;
   cat::verify(s[0] == 12_f4);
   float4x4 d = a - b;
   cat::verify(d[1] == 16_f4);
   float4x4 p = a * b;
   cat::verify(p[2] == 10_f4);
   float4x4 q = a / b;
   cat::verify(q[3] == 4_f4);

   float4x4 v = {1_f4, 2_f4, 3_f4, 4_f4};
   v += float4x4{1_f4, 1_f4, 1_f4, 1_f4};
   cat::verify(v[0] == 2_f4);
   v -= float4x4{1_f4, 1_f4, 1_f4, 1_f4};
   v *= float4x4{2_f4, 2_f4, 2_f4, 2_f4};
   cat::verify(v[3] == 8_f4);
   v /= float4x4{2_f4, 2_f4, 2_f4, 2_f4};
   cat::verify(v[1] == 2_f4);
}

test(simd_value_type_list_ctor) {
   int4x4 v(cat::value_type_list<int_lane, 5, 6, 7, 8>{});
   cat::verify(v[0] == 5);
   cat::verify(v[3] == 8);

   float4x4 fv(cat::value_type_list<float_lane, 1.25f, 2.25f, 3.25f, 4.25f>{});
   cat::verify(fv[0] == 1.25f);
   cat::verify(fv[3] == 4.25f);
}

// Loads/stores (pointer + idx): tests above:

// `simd_mask` constructors, fill, bitwise:

test(simd_mask_ctors_fill) {
   using M = int4x4::mask_type;
   M a{};
   M b = cat::make_simd_mask_filled<M>(true);
   cat::verify(b.all_of());
   M c = cat::make_simd_mask_filled<M>(false);
   cat::verify(c.none_of());

   M d{true, false, true, false};
   cat::verify(d[0] && !d[1] && d[2] && !d[3]);

   M e(cat::value_type_list<bool, false, true, false, true>{});
   cat::verify(!e[0] && e[1]);

   M f = cat::make_simd_mask_filled<M>(true);
   cat::verify(f.all_of());

   M g{};
   g.fill(false);
   cat::verify(g.none_of());
   g.fill(true);
   cat::verify(g.all_of());

   using float_mask = float4x4::mask_type;
   float_mask af{};
   float_mask bf = cat::make_simd_mask_filled<float_mask>(true);
   cat::verify(bf.all_of());
   float_mask df{true, false, true, false};
   cat::verify(df[0] && !df[1] && df[2] && !df[3]);
   float_mask ff = cat::make_simd_mask_filled<float_mask>(true);
   cat::verify(ff.all_of());
   float_mask gf{};
   gf.fill(false);
   cat::verify(gf.none_of());
}

test(simd_mask_bitwise_ops) {
   using M = int4x4::mask_type;
   M a{true, true, false, false};
   M b{true, false, true, false};
   M c = a & b;
   cat::verify(c[0] && !c[1] && !c[2]);
   M o = a | b;
   cat::verify(o[0] && o[1] && o[2]);
   M x = a ^ b;
   cat::verify(!x[0] && x[1] && x[2]);

   M u = a;
   u &= b;
   cat::verify(u[0] && !u[1]);
   u = a;
   u |= b;
   cat::verify(u[2]);
   u = a;
   u ^= b;
   cat::verify(!u[0]);

   using float_mask = float4x4::mask_type;
   float_mask a2{true, true, false, false};
   float_mask b2{true, false, true, false};
   float_mask c2 = a2 & b2;
   cat::verify(c2[0] && !c2[1] && !c2[2]);
   float_mask o2 = a2 | b2;
   cat::verify(o2[0] && o2[1] && o2[2]);
   float_mask x2 = a2 ^ b2;
   cat::verify(!x2[0] && x2[1] && x2[2]);
}

test(simd_mask_compare_eq_ne) {
   using M = int4x4::mask_type;
   M a{true, false, true, false};
   M b{true, true, false, false};
   M eq = a == b;
   cat::verify(eq[0] && !eq[1] && !eq[2] && eq[3]);
   M ne = a != b;
   cat::verify(!ne[0] && ne[1] && ne[2] && !ne[3]);

   using float_mask = float4x4::mask_type;
   float_mask af{true, false, true, false};
   float_mask bf{true, true, false, false};
   float_mask eqf = af == bf;
   cat::verify(eqf[0] && !eqf[1] && !eqf[2] && eqf[3]);
   float_mask nef = af != bf;
   cat::verify(!nef[0] && nef[1] && nef[2] && !nef[3]);
}

// P3922R1: CTAD `simd` from `simd_mask` matches `decltype(+mask)`. Unary +/-/~
// on masks yield integral `simd` with scalar-sized signed lanes.
test(simd_mask_ctad_and_integral_unaries) {
   int4x4 const a{1, 2, 3, 4};
   int4x4 const b{4, 2, 3, 1};
   auto const k = a < b;
   auto const unary_plus_k = +k;
   cat::simd const ctad_from_mask = k;
   static_assert(cat::is_same<cat::remove_const<decltype(ctad_from_mask)>,
                              cat::remove_const<decltype(unary_plus_k)>>);
   cat::verify(unary_plus_k == ctad_from_mask);
   cat::verify(unary_plus_k[0u] == 1 && unary_plus_k[1u] == 0
               && unary_plus_k[2u] == 0 && unary_plus_k[3u] == 0);

   auto const neg_k = -k;
   cat::verify(neg_k[0u] == -1 && neg_k[1u] == 0 && neg_k[2u] == 0
               && neg_k[3u] == 0);

   auto const tilde_k = ~k;
   cat::verify(tilde_k[0u] == -2 && tilde_k[1u] == -1 && tilde_k[2u] == -1
               && tilde_k[3u] == -1);

   auto const not_k = !k;
   cat::verify(!not_k[0] && not_k[1] && not_k[2] && not_k[3]);

   float4x4 const xf{1_f4, 2_f4, 3_f4, 4_f4};
   float4x4 const yf{4_f4, 2_f4, 3_f4, 1_f4};
   auto const kf = xf < yf;
   auto const pf = +kf;
   cat::simd const ctad_f = kf;
   static_assert(cat::is_same<cat::remove_const<decltype(ctad_f)>,
                              cat::remove_const<decltype(pf)>>);
   cat::verify(pf == ctad_f);
   cat::verify(pf[0u] == 1 && pf[1u] == 0 && pf[2u] == 0 && pf[3u] == 0);
}

// Element-wise logical && / || (draft `basic_mask`).
test(simd_mask_logical_and_or_lanes) {
   using M = int4x4::mask_type;
   M const a{true, false, true, false};
   M const b{true, true, false, false};
   M const land = a && b;
   cat::verify(land[0] && !land[1] && !land[2] && !land[3]);
   M const lor = a || b;
   cat::verify(lor[0] && lor[1] && lor[2] && !lor[3]);
}

test(simd_mask_to_bit_pattern) {
   using M = int4x4::mask_type;
   M a{5u};
   cat::verify(a.to_uword() == 5_uz);
   auto const bs = a.to_bitset();
   cat::verify(bs[0u]);
   cat::verify(!bs[1u]);
   cat::verify(bs[2u]);
   cat::verify(!bs[3u]);
   M b{0u};
   cat::verify(b.to_uword() == 0_uz);
   auto const zb = b.to_bitset();
   cat::verify(!zb[0u] && !zb[3u]);

   using float_mask = float4x4::mask_type;
   float_mask fa{5u};
   cat::verify(fa.to_uword() == 5_uz);
   auto const fbs = fa.to_bitset();
   cat::verify(fbs[0u] && !fbs[1u] && fbs[2u]);
}

test(simd_mask_load_and_loaded_from_bool_buffer) {
   using M = int4x4::mask_type;
   bool const lane_bools[] = {true, false, true, false};
   M b{};
   b.load(lane_bools);
   cat::verify(b[0] && !b[1] && b[2] && !b[3]);
   M const c = cat::make_simd_mask_loaded<M>(lane_bools);
   cat::verify(c[0] && !c[1] && c[2] && !c[3]);

   using float_mask = float4x4::mask_type;
   bool const float_lane_bools[] = {true, false, true, false};
   float_mask fb{};
   fb.load(float_lane_bools);
   cat::verify(fb[0] && !fb[1] && fb[2] && !fb[3]);
   float_mask const fc =
      cat::make_simd_mask_loaded<float_mask>(float_lane_bools);
   cat::verify(fc[0] && !fc[1] && fc[2] && !fc[3]);

   M const pass = cat::make_simd_mask_filled<M>(false);
   bool const reload[] = {true, false, true, true};
   M const lane_control = cat::make_simd_mask_from_count<int4x4>(2u);
   M const blended_load =
      cat::make_simd_mask_loaded<M>[lane_control](pass, reload);
   cat::verify(blended_load[0] && !blended_load[1] && !blended_load[2]
               && !blended_load[3]);

   M const pass_fill = cat::make_simd_mask_from_count<int4x4>(1u);
   M const blended_fill =
      cat::simd_mask_filled<M>[lane_control](pass_fill, true);
   cat::verify(blended_fill[0] && blended_fill[1] && !blended_fill[2]
               && !blended_fill[3]);
}

// `simd_mask` ` lane setter (P3275: no writable subscript):

test(simd_mask_set_lane) {
   using M = int4x4::mask_type;
   M m{};
   m.set_lane(0u, true);
   m.set_lane(2u, true);
   cat::verify(m[0] && !m[1] && m[2] && !m[3]);

   using float_mask = float4x4::mask_type;
   float_mask mf{};
   mf.set_lane(0u, true);
   mf.set_lane(2u, true);
   cat::verify(mf[0] && !mf[1] && mf[2] && !mf[3]);
}

// Mask queries (`cat::` and members):

test(simd_all_any_none_of) {
   using M = int4x4::mask_type;
   M t = cat::make_simd_mask_filled<M>(true);
   M f = cat::make_simd_mask_filled<M>(false);
   M m{true, false, false, false};
   cat::verify(t.all_of());
   cat::verify(t.any_of());
   cat::verify(!t.none_of());
   cat::verify(f.none_of());
   cat::verify(!f.any_of());
   cat::verify(m.any_of());
   cat::verify(!m.all_of());

   cat::verify(cat::simd_mask_all_of(t));
   cat::verify(cat::simd_mask_any_of(m));
   cat::verify(cat::simd_mask_none_of(f));

   using float_mask = float4x4::mask_type;
   float_mask tf = cat::make_simd_mask_filled<float_mask>(true);
   float_mask ff = cat::make_simd_mask_filled<float_mask>(false);
   float_mask mf2{true, false, false, false};
   cat::verify(tf.all_of());
   cat::verify(ff.none_of());
   cat::verify(mf2.any_of());
   cat::verify(cat::simd_mask_all_of(tf));
   cat::verify(cat::simd_mask_any_of(mf2));
   cat::verify(cat::simd_mask_none_of(ff));

   cat::verify(t.count_if_true() == 4u);
   cat::verify(f.count_if_true() == 0u);
   cat::verify(m.count_if_true() == 1u);
   cat::verify(m.find_if_true() == 0u);
   cat::verify(m.find_last_if_true() == 0u);
   cat::verify(t.find_if_true() == 0u);
   cat::verify(t.find_last_if_true() == 3u);

   M m_mid{false, false, true, false};
   cat::verify(m_mid.find_if_true() == 2u);
   cat::verify(m_mid.find_last_if_true() == 2u);
   M m_two{true, false, true, false};
   cat::verify(m_two.find_if_true() == 0u);
   cat::verify(m_two.find_last_if_true() == 2u);

   cat::verify(mf2.count_if_true() == 1u);
   cat::verify(mf2.find_if_true() == 0u);
   cat::verify(mf2.find_last_if_true() == 0u);
}

test(make_simd_mask_from_count_edges) {
   auto m0 = cat::make_simd_mask_from_count<int4x4>(0u);
   cat::verify(m0.none_of());
   auto m4 = cat::make_simd_mask_from_count<int4x4>(4u);
   cat::verify(m4.all_of());
   auto m2 = cat::make_simd_mask_from_count<int4x4>(2u);
   cat::verify(m2[0] && m2[1] && !m2[2] && !m2[3]);

   auto mf0 = cat::make_simd_mask_from_count<float4x4>(0u);
   cat::verify(mf0.none_of());
   auto mf4 = cat::make_simd_mask_from_count<float4x4>(4u);
   cat::verify(mf4.all_of());
   auto mf2 = cat::make_simd_mask_from_count<float4x4>(2u);
   cat::verify(mf2[0] && mf2[1] && !mf2[2] && !mf2[3]);
}

// Select, `if_else`, min, max:

test(simd_select_matches_if_else) {
   int4x4 a = {1, 2, 3, 4};
   int4x4 b = {10, 20, 30, 40};
   auto m = cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 r1 = cat::simd_if_else(m, a, b);
   int4x4 r2 = cat::simd_select(m, a, b);
   cat::verify(r1[0] == r2[0]);
   cat::verify(r1[3] == r2[3]);

   float4x4 fa = {1_f4, 2_f4, 3_f4, 4_f4};
   float4x4 fb = {10_f4, 20_f4, 30_f4, 40_f4};
   auto mf = cat::make_simd_mask_from_count<float4x4>(2u);
   float4x4 fr1 = cat::simd_if_else(mf, fa, fb);
   float4x4 fr2 = cat::simd_select(mf, fa, fb);
   cat::verify(fr1[0] == fr2[0]);
   cat::verify(fr1[3] == fr2[3]);
}

test(simd_min_max) {
   int4x4 a = {1, 5, 3, 8};
   int4x4 b = {4, 2, 7, 6};
   int4x4 lo = cat::simd_min(a, b);
   cat::verify(lo[0] == 1);
   cat::verify(lo[1] == 2);
   int4x4 hi = cat::simd_max(a, b);
   cat::verify(hi[0] == 4);
   cat::verify(hi[2] == 7);

   float4x4 x = {1_f4, 5_f4, 3_f4, 8_f4};
   float4x4 y = {4_f4, 2_f4, 7_f4, 6_f4};
   float4x4 mf = cat::simd_min(x, y);
   cat::verify(mf[1] == 2_f4);
   float4x4 xf = cat::simd_max(x, y);
   cat::verify(xf[3] == 8_f4);
}

// Permute, compress, expand, `load_from`, `store_to`:

test(simd_permute_dynamic) {
   int4x4 source_lanes = {10, 20, 30, 40};
   int4x4 permutation_indices{3, 2, 1, 0};
   int4x4 permuted = cat::simd_permute(source_lanes, permutation_indices);
   cat::verify(permuted[0] == 40);
   cat::verify(permuted[3] == 10);

   float4x4 float_source_lanes = {10_f4, 20_f4, 30_f4, 40_f4};
   float4x4 float_permutation_indices{3_f4, 2_f4, 1_f4, 0_f4};
   float4x4 float_permuted =
      cat::simd_permute(float_source_lanes, float_permutation_indices);
   cat::verify(float_permuted[0] == 40_f4);
   cat::verify(float_permuted[3] == 10_f4);
}

// Compile-time `simd_shuffle` and runtime-index `simd_permute` read the same
// source lanes when the shuffle vector matches the permute index vector (same
// relation sketched for `permute(v, indexes)` versus static shuffles in
// standard P2664).
test(simd_shuffle_equals_permute_for_matching_lane_indices) {
   int4x4 const src{10, 20, 30, 40};
   cat::verify(cat::simd_shuffle<3, 2, 1, 0>(src)
               == cat::simd_permute(src, int4x4{3, 2, 1, 0}));
   cat::verify(cat::simd_shuffle<1, 2, 3, 0>(src)
               == cat::simd_permute(src, int4x4{1, 2, 3, 0}));
   cat::verify(cat::simd_shuffle<0, 0, 3, 3>(src)
               == cat::simd_permute(src, int4x4{0, 0, 3, 3}));
   cat::verify(cat::simd_shuffle<0, 1, 2, 3>(src)
               == cat::simd_permute(src, int4x4{0, 1, 2, 3}));

   float4x4 const fv{10_f4, 20_f4, 30_f4, 40_f4};
   cat::verify(cat::simd_shuffle<3, 2, 1, 0>(fv)
               == cat::simd_permute(fv, float4x4{3_f4, 2_f4, 1_f4, 0_f4}));
}

test(simd_multi_subscript_permutes_via_simd_permute) {
   int4x4 const src{10, 20, 30, 40};
   cat::verify(src[3, 2, 1, 0] == cat::simd_permute(src, int4x4{3, 2, 1, 0}));
   cat::verify(src[int4x4{3, 2, 1, 0}]
               == cat::simd_permute(src, int4x4{3, 2, 1, 0}));

   float4x4 const fv{10_f4, 20_f4, 30_f4, 40_f4};
   cat::verify(fv[3, 2, 1, 0]
               == cat::simd_permute(fv, float4x4{3_f4, 2_f4, 1_f4, 0_f4}));

   auto narrow = src[1, 3];
   cat::verify(narrow.size() == 2u);
   cat::verify(narrow[0u] == src[1u]);
   cat::verify(narrow[1u] == src[3u]);
}

test(simd_concat_interleave_duplicate_reverse_blocks) {
   int4x4 lower_half_lanes = {1, 2, 3, 4};
   int4x4 upper_half_lanes = {5, 6, 7, 8};
   int4x8 concatenated_lanes =
      cat::simd_concat(lower_half_lanes, upper_half_lanes);
   cat::verify(concatenated_lanes[0] == 1);
   cat::verify(concatenated_lanes[3] == 4);
   cat::verify(concatenated_lanes[4] == 5);
   cat::verify(concatenated_lanes[7] == 8);

   int4x8 interleaved_lanes =
      cat::simd_interleave(lower_half_lanes, upper_half_lanes);
   cat::verify(interleaved_lanes[0] == 1);
   cat::verify(interleaved_lanes[1] == 5);
   cat::verify(interleaved_lanes[6] == 4);
   cat::verify(interleaved_lanes[7] == 8);

   int4x4 duplicate_even_source = {10, 20, 30, 40};
   int4x4 duplicate_even_lanes =
      cat::simd_duplicate_even(duplicate_even_source);
   cat::verify(duplicate_even_lanes[0] == 10);
   cat::verify(duplicate_even_lanes[1] == 10);
   cat::verify(duplicate_even_lanes[2] == 30);
   cat::verify(duplicate_even_lanes[3] == 30);

   int4x4 duplicate_odd_source = {10, 20, 30, 40};
   int4x4 duplicate_odd_lanes = cat::simd_duplicate_odd(duplicate_odd_source);
   cat::verify(duplicate_odd_lanes[0] == 20);
   cat::verify(duplicate_odd_lanes[1] == 20);
   cat::verify(duplicate_odd_lanes[2] == 40);
   cat::verify(duplicate_odd_lanes[3] == 40);

   int4x4 reverse_blocks_source = {1, 2, 3, 4};
   int4x4 reverse_blocks_result =
      cat::simd_reverse_blocks(reverse_blocks_source, 2u);
   cat::verify(reverse_blocks_result[0] == 2);
   cat::verify(reverse_blocks_result[1] == 1);
   cat::verify(reverse_blocks_result[2] == 4);
   cat::verify(reverse_blocks_result[3] == 3);

   float4x4 float_lower_half_lanes = {1_f4, 2_f4, 3_f4, 4_f4};
   float4x4 float_upper_half_lanes = {5_f4, 6_f4, 7_f4, 8_f4};
   float4x8 float_concatenated_lanes =
      cat::simd_concat(float_lower_half_lanes, float_upper_half_lanes);
   cat::verify(float_concatenated_lanes[0] == 1_f4);
   cat::verify(float_concatenated_lanes[7] == 8_f4);
   float4x8 float_interleaved_lanes =
      cat::simd_interleave(float_lower_half_lanes, float_upper_half_lanes);
   cat::verify(float_interleaved_lanes[0] == 1_f4);
   cat::verify(float_interleaved_lanes[1] == 5_f4);
   float4x4 float_duplicate_even_source = {10_f4, 20_f4, 30_f4, 40_f4};
   float4x4 float_duplicate_even_lanes =
      cat::simd_duplicate_even(float_duplicate_even_source);
   cat::verify(float_duplicate_even_lanes[2] == 30_f4);
   float4x4 float_duplicate_odd_source = {10_f4, 20_f4, 30_f4, 40_f4};
   float4x4 float_duplicate_odd_lanes =
      cat::simd_duplicate_odd(float_duplicate_odd_source);
   cat::verify(float_duplicate_odd_lanes[3] == 40_f4);
}

test(simd_compress_expand) {
   int4x4 source_values = {1, 2, 3, 4};
   auto first_two_lanes_true_mask = cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 compressed_lanes =
      cat::simd_compress(source_values, first_two_lanes_true_mask);
   cat::verify(compressed_lanes[0] == 1);
   cat::verify(compressed_lanes[1] == 2);
   cat::verify(compressed_lanes[2] == 0);

   int4x4 expand_background = {9, 9, 9, 9};
   int4x4 expand_packed_input = {100, 200, 0, 0};
   int4x4 expanded_lanes = cat::simd_expand(
      expand_packed_input, first_two_lanes_true_mask, expand_background);
   cat::verify(expanded_lanes[0] == 100);
   cat::verify(expanded_lanes[1] == 200);
   cat::verify(expanded_lanes[2] == 9);
   cat::verify(expanded_lanes[3] == 9);

   float4x4 float_source_values = {1_f4, 2_f4, 3_f4, 4_f4};
   auto float_first_two_lanes_true_mask =
      cat::make_simd_mask_from_count<float4x4>(2u);
   float4x4 float_compressed_lanes =
      cat::simd_compress(float_source_values, float_first_two_lanes_true_mask);
   cat::verify(float_compressed_lanes[0] == 1_f4);
   cat::verify(float_compressed_lanes[1] == 2_f4);
   float4x4 float_expand_background = {9_f4, 9_f4, 9_f4, 9_f4};
   float4x4 float_expand_packed_input = {100_f4, 200_f4, 0_f4, 0_f4};
   float4x4 float_expanded_lanes = cat::simd_expand(
      float_expand_packed_input, float_first_two_lanes_true_mask,
      float_expand_background);
   cat::verify(float_expanded_lanes[0] == 100_f4);
   cat::verify(float_expanded_lanes[1] == 200_f4);
   cat::verify(float_expanded_lanes[2] == 9_f4);
}

test(simd_load_from_store_to) {
   cat::int4 contiguous_int_source[8] = {100, 101, 102, 103,
                                         104, 105, 106, 107};
   int4x4 gather_index_lanes{0, 2, 4, 1};
   auto gathered_int_lanes =
      cat::simd_load_from<cat::int4, cat::fixed_size_abi<cat::int4, 4u>>(
         contiguous_int_source, gather_index_lanes);
   cat::verify(gathered_int_lanes[0] == 100);
   cat::verify(gathered_int_lanes[1] == 102);
   cat::verify(gathered_int_lanes[2] == 104);
   cat::verify(gathered_int_lanes[3] == 101);

   cat::int4 scatter_destination_int[8] = {};
   int4x4 scatter_index_lanes{5, 3, 1, 6};
   int4x4 scatter_value_lanes = {11, 22, 33, 44};
   cat::simd_store_to<cat::int4, cat::fixed_size_abi<cat::int4, 4u>>(
      scatter_destination_int, scatter_index_lanes, scatter_value_lanes);
   cat::verify(scatter_destination_int[5] == 11);
   cat::verify(scatter_destination_int[3] == 22);
   cat::verify(scatter_destination_int[1] == 33);
   cat::verify(scatter_destination_int[6] == 44);

   cat::float4 contiguous_float_source[8] = {100_f4, 101_f4, 102_f4, 103_f4,
                                             104_f4, 105_f4, 106_f4, 107_f4};
   float4x4 float_gather_index_lanes{0_f4, 2_f4, 4_f4, 1_f4};
   auto gathered_float_lanes =
      cat::simd_load_from<cat::float4, cat::fixed_size_abi<cat::float4, 4u>>(
         contiguous_float_source, float_gather_index_lanes);
   cat::verify(gathered_float_lanes[0] == 100.f);
   cat::verify(gathered_float_lanes[2] == 104.f);
   cat::float4 scatter_destination_float[8] = {};
   float4x4 float_scatter_index_lanes{5_f4, 3_f4, 1_f4, 6_f4};
   cat::simd_store_to<cat::float4, cat::fixed_size_abi<cat::float4, 4u>>(
      scatter_destination_float, float_scatter_index_lanes,
      float4x4{11_f4, 22_f4, 33_f4, 44_f4});
   cat::verify(scatter_destination_float[5] == 11_f4);
   cat::verify(scatter_destination_float[3] == 22_f4);

   auto const regathered_int_lanes =
      cat::simd_load_from<cat::int4, cat::fixed_size_abi<cat::int4, 4u>>(
         contiguous_int_source, gather_index_lanes);
   cat::verify(regathered_int_lanes[0] == gathered_int_lanes[0]
               && regathered_int_lanes[3] == gathered_int_lanes[3]);

   int4x4 const gather_masked_passthrough = {1, 2, 3, 4};
   auto const gather_active_lane_mask =
      cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 const masked_gather_lanes = cat::simd_load_from<
      cat::int4, cat::fixed_size_abi<cat::int4, 4u>>[gather_active_lane_mask](
      gather_masked_passthrough, contiguous_int_source, gather_index_lanes);
   cat::verify(masked_gather_lanes[0] == 100 && masked_gather_lanes[1] == 102
               && masked_gather_lanes[2] == 3 && masked_gather_lanes[3] == 4);

   cat::int4 partial_scatter_destination[8] = {9, 9, 9, 9, 9, 9, 9, 9};
   int4x4 const partial_scatter_values = {11, 22, 33, 44};
   cat::simd_store_to<
      cat::int4, cat::fixed_size_abi<
                    cat::int4, 4u>>[cat::make_simd_mask_from_count<int4x4>(2u)](
      partial_scatter_destination, scatter_index_lanes, partial_scatter_values);
   cat::verify(partial_scatter_destination[5] == 11
               && partial_scatter_destination[3] == 22
               && partial_scatter_destination[1] == 9
               && partial_scatter_destination[6] == 9);

   alignas(int4x4::abi_type::alignment.raw)
      int_lane const aligned_int_source[] = {11, 22, 33, 44};
   int4x4 const aligned_loaded_lanes =
      cat::make_simd_loaded_aligned<int4x4>(aligned_int_source).verify();
   cat::verify(aligned_loaded_lanes[0] == 11 && aligned_loaded_lanes[3] == 44);

   int4x4 const masked_load_passthrough = {10, 20, 30, 40};
   int4x4 const masked_make_loaded_aligned =
      cat::make_simd_loaded_aligned<int4x4>[gather_active_lane_mask](
         masked_load_passthrough, aligned_int_source)
         .verify();
   int4x4 const masked_simd_load_aligned =
      cat::simd_load_aligned[gather_active_lane_mask](masked_load_passthrough,
                                                      aligned_int_source)
         .verify();
   cat::verify(masked_make_loaded_aligned[0] == masked_simd_load_aligned[0]
               && masked_make_loaded_aligned[2] == masked_simd_load_aligned[2]);

   int4x4 const filled_lanes = cat::make_simd_filled<int4x4>(7);
   cat::verify(filled_lanes[2] == 7);
   int4x4 const filled_masked_passthrough = {1, 2, 3, 4};
   int4x4 const masked_filled_lanes =
      cat::simd_filled<int4x4>[gather_active_lane_mask](
         filled_masked_passthrough, 99);
   cat::verify(masked_filled_lanes[0] == 99 && masked_filled_lanes[1] == 99
               && masked_filled_lanes[2] == 3 && masked_filled_lanes[3] == 4);
}

// `simd_bit_cast_as`, `.raw` round-trip:

test(simd_bit_cast_as_unsigned) {
   int4x4 signed_source_lanes = {-1, 0, 1, -2};
   auto unsigned_reinterpret_lanes =
      cat::simd_bit_cast_as<uint_lane>(signed_source_lanes);
   cat::verify(unsigned_reinterpret_lanes[0] == static_cast<uint_lane>(-1));
   cat::verify(unsigned_reinterpret_lanes[2] == 1u);

   float4x4 float_source_lanes = {-1.f, 0.f, 1.f, -2.f};
   auto float_bits_as_unsigned_lanes =
      cat::simd_bit_cast_as<uint_lane>(float_source_lanes);
   cat::verify(float_bits_as_unsigned_lanes[0]
               == __builtin_bit_cast(uint_lane, -1.f));
   cat::verify(float_bits_as_unsigned_lanes[2]
               == __builtin_bit_cast(uint_lane, 1.f));
}

test(simd_raw_roundtrip) {
   int4x4 int_roundtrip_source = {7, 8, 9, 10};
   auto int_vector_raw = int_roundtrip_source.raw;
   int4x4 int_roundtrip_rebuilt(int_vector_raw);
   cat::verify(int_roundtrip_rebuilt[1] == 8);

   float4x4 float_roundtrip_source = {7.f, 8.f, 9.f, 10.f};
   auto float_vector_raw = float_roundtrip_source.raw;
   float4x4 float_roundtrip_rebuilt(float_vector_raw);
   cat::verify(float_roundtrip_rebuilt[1] == 8.f);
}

// Reductions (`__builtin_reduce_*`, P0918R1 context):

test(simd_reduce_add_mul_and_or_xor) {
   int4x4 const a = {10, 20, 30, 40};
   cat::verify(a.reduce_add()[0u] == 100);
   cat::verify(cat::simd_reduce_add(a)[0u] == 100);
   cat::verify(a.sum() == 100);
   cat::verify(a.sum() == a.reduce_add()[0u]);

   int4x4 const m = {2, 3, -1, 1};
   cat::verify(m.reduce_mul()[0u] == -6);
   cat::verify(cat::simd_reduce_mul(m)[0u] == -6);
   cat::verify(m.multiply() == -6);
   cat::verify(m.multiply() == m.reduce_mul()[0u]);

   uint4x4 const p = {2u, 5u, 1u, 3u};
   cat::verify(p.sum() == 11u);
   cat::verify(p.multiply() == 30u);

   uint4x4 const x = {0xfu, 0xffu, 0u, 0xffffu};
   cat::verify(x.reduce_and()[0u] == 0u);
   cat::verify(cat::simd_reduce_and(x)[0u] == 0u);
   cat::verify(x.reduce_or()[0u] == 0xffffu);
   cat::verify(x.reduce_xor()[0u] == (0xfu ^ 0xffu ^ 0u ^ 0xffffu));
}

test(simd_reduce_min_max_integral) {
   int4x4 const v = {3, -5, 7, 1};
   cat::verify(v.reduce_max()[0u] == 7);
   cat::verify(v.reduce_min()[0u] == -5);
   cat::verify(cat::simd_reduce_max(v)[0u] == 7);
   cat::verify(cat::simd_reduce_min(v)[0u] == -5);

   uint4x4 const u = {9u, 2u, 11u, 4u};
   cat::verify(u.reduce_max()[0u] == 11u);
   cat::verify(u.reduce_min()[0u] == 2u);
}

test(simd_reduce_min_max_maximum_minimum_float) {
   float4x4 const f = {1_f4, -2_f4, 5_f4, 3_f4};
   cat::verify(f.reduce_max()[0u] == 5_f4);
   cat::verify(f.reduce_min()[0u] == -2_f4);
   cat::verify(cat::simd_reduce_max(f)[0u] == 5_f4);
   cat::verify(cat::simd_reduce_min(f)[0u] == -2_f4);

   cat::verify(f.reduce_maximum()[0u] == 5_f4);
   cat::verify(f.reduce_minimum()[0u] == -2_f4);
   cat::verify(cat::simd_reduce_maximum(f)[0u] == 5_f4);
   cat::verify(cat::simd_reduce_minimum(f)[0u] == -2_f4);

   float4x4 const nanish = {__builtin_nanf(""), 2_f4, 3_f4, 4_f4};
   cat::verify(nanish.reduce_max()[0u] == 4_f4);
   cat::verify(nanish.reduce_min()[0u] == 2_f4);
   // `reduce_max`/`reduce_min` ignore `NaN` unless every lane is `NaN`. IEEE
   // `reduce_maximum`/`reduce_minimum` follow `maximumNumber`/`minimumNumber`
   // and can propagate `NaN` from a lane.
   cat::verify(
      __builtin_isnan(make_raw_arithmetic(nanish.reduce_maximum()[0u])));
   cat::verify(
      __builtin_isnan(make_raw_arithmetic(nanish.reduce_minimum()[0u])));
}

test(simd_reduce_assoc_and_in_order_fadd) {
   float4x4 const v = {1_f4, 2_f4, 3_f4, 4_f4};
   cat::verify(v.reduce_assoc_fadd()[0u] == 10_f4);
   cat::verify(cat::simd_reduce_assoc_fadd(v)[0u] == 10_f4);
   cat::verify(v.reduce_assoc_fadd(100_f4)[0u] == 110_f4);
   cat::verify(cat::simd_reduce_assoc_fadd(v, 100_f4)[0u] == 110_f4);
   cat::verify(v.reduce_in_order_fadd(0_f4)[0u] == 10_f4);
   cat::verify(cat::simd_reduce_in_order_fadd(v, 0_f4)[0u] == 10_f4);
   cat::verify(v.reduce_in_order_fadd(100_f4)[0u] == 110_f4);

   float8x2 const d = {1.5, 2.5};
   cat::verify(d.reduce_assoc_fadd()[0u] == 4.0);
   cat::verify(d.reduce_in_order_fadd(10.0)[0u] == 14.0);
}

test(simd_mask_reduce_integral) {
   using M = int4x4::mask_type;
   M const a{true, false, true, true};
   cat::verify(a.count_if_true() == 3u);
   cat::verify(a.find_if_true() == 0u);
   cat::verify(a.find_last_if_true() == 3u);
}

// <bit>-style lane ops (integral scalar lanes only: popcount, rotate:
// left, rotate right):

test(simd_popcount_rotate_left_rotate_right) {
   cat::simd<uint_lane, cat::fixed_size_abi<uint_lane, 4u>> v{0u, 3u, 15u,
                                                              0xffffffffu};
   auto pc = cat::simd_popcount(v);
   cat::verify(v.popcount() == pc);
   cat::verify(pc[0] == 0u);
   cat::verify(pc[1] == 2u);
   cat::verify(pc[2] == 4u);
   cat::verify(pc[3] == 32u);

   using mi4 = cat::simd_mask<cat::int4, cat::fixed_size_abi<cat::int4, 4u>>;
   mi4 mb{true, false, true, false};
   cat::verify(cat::popcount(mb) == 2);

   uint4x4 rx = {1u, 2u, 4u, 8u};
   uint4x4 rotated_left = cat::simd_rotate_left(rx, 1);
   cat::verify(rotated_left[0] == 2u);
   uint4x4 rotated_right = cat::simd_rotate_right(rx, 1);
   cat::verify(rotated_right[3] == 4u);
}

// EVE-style mask subscript: `fn[mask](args?)` applies `fn` only where the:
// mask is true. Inactive lanes keep the first SIMD argument (`v`, or `a` for
// binary):

test(simd_eve_mask_subscript_sqrt_cbrt_nroot_float) {
   float4x4 v = {1_f4, 4_f4, 9_f4, 16_f4};
   auto m = cat::make_simd_mask_from_count<float4x4>(2u);
   float4x4 r = cat::simd_sqrt[m](v);
   cat::verify(r[0] == 1_f4);
   cat::verify(r[1] == 2_f4);
   cat::verify(r[2] == 9_f4);
   cat::verify(r[3] == 16_f4);

   float4x4 const n2 = cat::simd_nroot(v, 2);
   cat::verify(n2 == cat::simd_sqrt(v));

   float4x4 cubes = {1_f4, 8_f4, 27_f4, 64_f4};
   float4x4 cb = cat::simd_cbrt[m](cubes);
   cat::verify(cb[0] == 1_f4);
   cat::verify(cb[1] == 2_f4);
   cat::verify(cb[2] == 27_f4);
   cat::verify(cb[3] == 64_f4);
   float4x4 cb_u = cat::simd_cbrt(cubes);
   cat::verify(cb_u == cat::simd_nroot(cubes, 3));

   float4x4 fourths = {1_f4, 16_f4, 81_f4, 256_f4};
   float4x4 nr = cat::simd_nroot[m](fourths, 4);
   cat::verify(nr[0] == 1_f4);
   cat::verify(nr[1] == 2_f4);
   cat::verify(nr[2] == 81_f4);
   cat::verify(nr[3] == 256_f4);
}

test(simd_eve_mask_subscript_min_max_popcount_rotate_left_rotate_right) {
   float4x4 fx = {1_f4, 5_f4, 3_f4, 8_f4};
   float4x4 fy = {4_f4, 2_f4, 7_f4, 6_f4};
   auto mf = cat::make_simd_mask_from_count<float4x4>(2u);
   float4x4 flo = cat::simd_min[mf](fx, fy);
   cat::verify(flo[0] == 1_f4);
   cat::verify(flo[1] == 2_f4);
   cat::verify(flo[2] == 3_f4);
   cat::verify(flo[3] == 8_f4);
   float4x4 fhi = cat::simd_max[mf](fx, fy);
   cat::verify(fhi[0] == 4_f4);
   cat::verify(fhi[1] == 5_f4);
   cat::verify(fhi[2] == 3_f4);
   cat::verify(fhi[3] == 8_f4);

   int4x4 a = {1, 5, 3, 8};
   int4x4 b = {4, 2, 7, 6};
   auto m = cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 lo = cat::simd_min[m](a, b);
   cat::verify(lo[0] == 1);
   cat::verify(lo[1] == 2);
   cat::verify(lo[2] == 3);
   cat::verify(lo[3] == 8);
   int4x4 hi = cat::simd_max[m](a, b);
   cat::verify(hi[0] == 4);
   cat::verify(hi[1] == 5);
   cat::verify(hi[2] == 3);
   cat::verify(hi[3] == 8);

   using u32x4 = cat::simd<uint_lane, cat::fixed_size_abi<uint_lane, 4u>>;
   u32x4 w{0u, 3u, 15u, 0xffffffffu};
   cat::verify(w.popcount() == cat::simd_popcount(w));
   auto mu = cat::make_simd_mask_from_count<u32x4>(2u);
   auto pc = cat::simd_popcount[mu](w);
   cat::verify(pc[0] == 0u);
   cat::verify(pc[1] == 2u);
   cat::verify(pc[2] == 15u);
   cat::verify(pc[3] == 0xffffffffu);

   uint4x4 rx = {1u, 2u, 4u, 8u};
   auto rm = cat::make_simd_mask_from_count<uint4x4>(2u);
   uint4x4 rotated_left = cat::simd_rotate_left[rm](rx, 1);
   cat::verify(rotated_left[0] == 2u);
   cat::verify(rotated_left[1] == 4u);
   cat::verify(rotated_left[2] == 4u);
   cat::verify(rotated_left[3] == 8u);
   uint4x4 ry = {8u, 4u, 2u, 1u};
   uint4x4 rotated_right = cat::simd_rotate_right[rm](ry, 1);
   cat::verify(rotated_right[0] == 4u);
   cat::verify(rotated_right[1] == 2u);
   cat::verify(rotated_right[2] == 2u);
   cat::verify(rotated_right[3] == 1u);
}

// `chunked_invoke`, `vectorizable_element`:

test(simd_chunked_invoke) {
   iword seen = iword(0);
   int4x4 pack = {1, 2, 3, 4};
   cat::simd_chunked_invoke(
      [&](int4x4 x) {
         seen = seen + iword(x[0] + x[3]);
      },
      pack);
   cat::verify(seen == iword(5));

   cat::float4 fseen = 0_f4;
   float4x4 fpack = {1_f4, 2_f4, 3_f4, 4_f4};
   cat::simd_chunked_invoke(
      [&](float4x4 x) {
         fseen = fseen + (x[0] + x[3]);
      },
      fpack);
   cat::verify(fseen == 5_f4);
}

test(simd_concepts) {
   static_assert(cat::is_simd<int4x4>);
   static_assert(cat::is_simd<float4x4>);
   static_assert(!cat::is_simd<int_lane>);
   static_assert(cat::is_simd_mask<int4x4::mask_type>);
   static_assert(cat::is_simd_or_mask<int4x4>);
   static_assert(cat::is_simd_or_mask<int4x4::mask_type>);
   static_assert(cat::is_simd_integral<int4x4>);
   static_assert(!cat::is_simd_integral<float4x4>);
   static_assert(cat::vec_floating_point<float4x4>);
   static_assert(!cat::vec_floating_point<int4x4>);
   static_assert(cat::is_simd_of<int4x4, cat::int4>);
   static_assert(!cat::is_simd_of<int4x4, cat::float4>);
   static_assert(cat::vectorizable_element<int_lane>);
   static_assert(cat::vectorizable_element<double_lane>);
}

// `vectorized_stepanov_iterator` (Vc simdize iterator-style coverage):
test(simd_as_vectorized_iterate_twice) {
   int_lane const data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
   auto it = cat::as_vectorized<int4, 4u>(data);
   auto it2 = cat::as_vectorized<int4, 4u>(data + 4);
   cat::verify(it != it2);
   int4x4 a = *it;
   cat::verify(a[3] == 4);
   ++it;
   int4x4 b = *it;
   cat::verify(b[0] == 5);
   cat::verify(b[3] == 8);
   cat::verify(it == it2);

   float_lane const fdata[8] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
   auto fit = cat::as_vectorized<float4, 4u>(fdata);
   auto fit2 = cat::as_vectorized<float4, 4u>(fdata + 4);
   cat::verify(fit != fit2);
   float4x4 fa = *fit;
   cat::verify(fa[3] == 4.f);
   ++fit;
   float4x4 fb = *fit;
   cat::verify(fb[0] == 5.f);
   cat::verify(fb[3] == 8.f);
   cat::verify(fit == fit2);
}

test(simd_as_vectorized_random_access_and_mutation) {
   auto const sum4 = [](float4x4 v) -> cat::float4 {
      return v[0] + v[1] + v[2] + v[3];
   };

   static constexpr idx k_extent = 128u;
   float_lane buf[128] = {};
   for (idx i = 0u; i < k_extent; ++i) {
      buf[i.raw] = float_lane(static_cast<float>(k_extent.raw - i.raw));
   }

   auto b = cat::as_vectorized<float4, 4u>(buf);
   auto e = cat::as_vectorized<float4, 4u>(buf + k_extent.raw);
   auto const& bconst = b;

   float4x4 const ref0 = float4x4(float_lane(static_cast<float>(k_extent.raw)))
                         - cat::iota<float4x4>(0_f4);
   cat::verify(sum4(*b) == sum4(ref0));
   cat::verify(sum4(*bconst) == sum4(ref0));

   float4x4 const ref_last = 4.f - cat::iota<float4x4>(0_f4);
   cat::verify(sum4(*(e - 1)) == sum4(ref_last));
   cat::verify(sum4(*(e - 1)) == sum4(ref_last));

   float4x4 const ref_second_chunk =
      float4x4(float_lane(static_cast<float>(k_extent.raw - 4u)))
      - cat::iota<float4x4>(0_f4);
   cat::verify(sum4(*(b + 1)) == sum4(ref_second_chunk));
   cat::verify(sum4(*(b + 1)) == sum4(ref_second_chunk));

   iword const chunk_count =
      iword(static_cast<iword::raw_type>(k_extent.raw / 4uz));
   cat::verify(e - b == chunk_count);
   cat::verify(b - e == -chunk_count);

   cat::verify(b < e);
   cat::verify(!(b > e));
   cat::verify(e > b);
   cat::verify(!(e < b));
   cat::verify(b <= e);
   cat::verify(!(b >= e));
   cat::verify(e >= b);
   cat::verify(!(e <= b));
   cat::verify(b != e);
   cat::verify(!(b == e));

   auto next = b + 1;
   cat::verify(next > b);
   cat::verify(!(b > next));
   cat::verify(!(next < b));
   cat::verify(b < next);
   cat::verify(next >= b);
   cat::verify(!(b >= next));
   cat::verify(!(next <= b));
   cat::verify(b <= next);
   cat::verify(b != next);
   cat::verify(!(b == next));

   next--;
   cat::verify(next == b);
   cat::verify(sum4(*next) == sum4(*b));

   float_lane ref_hi = float_lane(static_cast<float>(k_extent.raw));
   for (auto cur = b; cur != e; ++cur, ref_hi -= float_lane(4.f)) {
      float4x4 x = *cur;
      float4x4 const ref = float4x4(ref_hi) - cat::iota<float4x4>(0_f4);
      cat::verify(sum4(x) == sum4(ref));
      cat::verify(sum4(*cur) == sum4(ref));
      *cur = x + 1_f4;
      cat::verify(sum4(*cur) == sum4(ref + 1_f4));
   }

   ref_hi = float_lane(static_cast<float>(k_extent.raw)) + 1.f;
   for (auto cur = b; cur != e; ++cur, ref_hi -= float_lane(4.f)) {
      float4x4 x = *cur;
      float4x4 const ref = float4x4(ref_hi) - cat::iota<float4x4>(0_f4);
      cat::verify(sum4(x) == sum4(ref));
      cat::verify(sum4(*cur) == sum4(ref));
   }

   float_lane const* cbuf = buf;
   float_lane ref_c = float_lane(static_cast<float>(k_extent.raw)) + 1.f;
   for (auto cit = cat::as_vectorized<float4, 4u>(cbuf),
             cend = cat::as_vectorized<float4, 4u>(cbuf + k_extent.raw);
        cit != cend; ++cit, ref_c -= float_lane(4.f)) {
      float4x4 x = *cit;
      float4x4 const ref = float4x4(ref_c) - cat::iota<float4x4>(0_f4);
      cat::verify(sum4(x) == sum4(ref));
      cat::verify(sum4(*cit) == sum4(ref));
   }

   float4x4 const ref0_after = ref0 + 1_f4;
   float4x4 const ref_second_after = ref_second_chunk + 1_f4;
   cat::verify(sum4(b[0]) == sum4(ref0_after));
   cat::verify(sum4(b[1]) == sum4(ref_second_after));
}

test(simd_swap_lanes) {
   float4x4 a = {1_f4, 2_f4, 3_f4, 4_f4};
   float4x4 b = {10_f4, 20_f4, 30_f4, 40_f4};
   float4x4 const saved_a = a;
   float4x4 const saved_b = b;
   cat::swap(a, b);
   cat::verify(a[0] == saved_b[0] && a[3] == saved_b[3]);
   cat::verify(b[0] == saved_a[0] && b[3] == saved_a[3]);
   cat::swap(a, a);
   cat::verify(a[0] == saved_b[0]);

   int4x4 i = {1, 2, 3, 4};
   int4x4 j = {100, 200, 300, 400};
   int4x4 const saved_i = i;
   cat::swap(i, j);
   cat::verify(i[0] == 100 && j[0] == saved_i[0]);

   using M = int4x4::mask_type;
   using float_mask = float4x4::mask_type;
   bool const ab[] = {true, false, true, false};
   bool const cd[] = {false, true, false, true};
   M m1 = cat::make_simd_mask_loaded<M>(ab);
   M m2 = cat::make_simd_mask_loaded<M>(cd);
   bool const saved_m1_0 = m1[0];
   bool const saved_m2_0 = m2[0];
   cat::swap(m1, m2);
   cat::verify(m1[0] == saved_m2_0);
   cat::verify(m2[0] == saved_m1_0);

   bool const fab[] = {true, true, false, false};
   bool const fcd[] = {false, false, true, true};
   float_mask mf1 = cat::make_simd_mask_loaded<float_mask>(fab);
   float_mask mf2 = cat::make_simd_mask_loaded<float_mask>(fcd);
   bool const saved_mf1_2 = mf1[2];
   cat::swap(mf1, mf2);
   cat::verify(mf1[2] != saved_mf1_2);
}

test(simd_vc_style_bool_masks_and_lane_types) {
   static_assert(cat::is_simd_mask<int4x4::mask_type>);
   static_assert(cat::is_simd_mask<float4x4::mask_type>);
   static_assert(cat::is_same<std::tuple_element_t<0u, int4x4::mask_type>,
                              int4x4::mask_type::lane_scalar>);
   static_assert(cat::is_same<std::tuple_element_t<0u, float4x4::mask_type>,
                              float4x4::mask_type::lane_scalar>);

   using int4_mask = int4x4::mask_type;
   using float_mask = float4x4::mask_type;
   bool const lane_bools[] = {true, false, false, true};
   int4_mask m = cat::make_simd_mask_loaded<int4_mask>(lane_bools);
   cat::verify(m[0] && !m[1] && !m[2] && m[3]);
   float_mask mf = cat::make_simd_mask_loaded<float_mask>(lane_bools);
   cat::verify(mf[0] && !mf[1]);
}

test(simd_as_vectorized_over_vec_float_data) {
   auto const sum4 = [](float4x4 v) -> cat::float4 {
      return v[0] + v[1] + v[2] + v[3];
   };

   static constexpr idx k_extent = 128u;
   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(8_uki).or_exit();
   defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);
   cat::vec<float_lane, cat::linear_allocator> storage =
      cat::make_vec<float_lane>(allocator);
   storage.reserve(k_extent).or_exit();
   for (idx i = 0u; i < k_extent; ++i) {
      storage.push_back(float_lane(static_cast<float>(k_extent.raw - i.raw)))
         .or_exit();
   }
   cat::verify(storage.size() == k_extent);

   auto b = cat::as_vectorized<float4, 4u>(storage.data());
   float4x4 const ref0 = float4x4(float_lane(static_cast<float>(k_extent.raw)))
                         - cat::iota<float4x4>(0_f4);
   cat::verify(sum4(*b) == sum4(ref0));
   cat::verify(
      sum4(*(b + 1))
      == sum4(float4x4(float_lane(static_cast<float>(k_extent.raw - 4u)))
              - cat::iota<float4x4>(0_f4)));
}

// TODO: Claude is too dumb to write this. Do it yourself.
#if 0
test(simd_list_stepanov_iterator_vectorization_float) {
   // Mirrors Vc `list_iterator_vectorization` (`float` `std::list` block).
   // `list_iter` models `simdize<L::iterator>`. `e` is `++list.end()` because
   // `cat::list` keeps the last node as `end()` and advancing past the tail
   // matches a simdized past-end iterator.
   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(32_uki).or_exit();
   defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   using L = float_list;
   L list = cat::make_list<float_lane>(allocator).verify();
   for (idx i = 1'024u; i != 0u; i = idx(i.raw - 1u)) {
      list.push_back(float_lane(static_cast<float>(i.raw))).verify();
   }

   using list_iter = float_v_list_iter;
   list_iter b = list.begin();
   list_iter const e = simd_float_v_list_end(list);

   float4x4 reference =
      float4x4(float_lane(static_cast<float>(list.size().raw)))
      - cat::iota<float4x4>(0_f4);
   for (; b != e; ++b, reference = reference - 4_f4) {
      float4x4 const x = *b;
      verify_float4x4_eq(x, reference);
      verify_float4x4_eq(*b, reference);
      *b = x + 1_f4;
      verify_float4x4_eq(*b, reference + 1_f4);
      auto&& ref = *b;
      ref = x + 2_f4;
      verify_float4x4_eq(*b, reference + 2_f4);
      verify_float4x4_eq(static_cast<float4x4>(ref),
                         reference + 2_f4);
      ref = x + 1_f4;
      verify_float4x4_eq(*b, reference + 1_f4);
      verify_float4x4_eq(static_cast<float4x4>(ref),
                         reference + 1_f4);
   }

   reference = float4x4(float_lane(static_cast<float>(list.size().raw)))
               - cat::iota<float4x4>(0_f4) + 1_f4;
   for (b = list.begin(); b != e; ++b, reference = reference - 4_f4) {
      float4x4 const x = *b;
      verify_float4x4_eq(x, reference);
      verify_float4x4_eq(*b, reference);
   }
}
#endif

// EVE-style conditionals:

test(simd_ignore_first_last_keep_between) {
   auto a = cat::make_simd_mask_ignore_first<int4x4>(1u);
   cat::verify(!a[0] && a[1] && a[3]);
   auto b = cat::make_simd_mask_ignore_last<int4x4>(1u);
   cat::verify(b[0] && b[2] && !b[3]);
   auto c = cat::make_simd_mask_keep_between<int4x4>(1u, 3u);
   cat::verify(!c[0] && c[1] && c[2] && !c[3]);

   auto fa = cat::make_simd_mask_ignore_first<float4x4>(1u);
   cat::verify(!fa[0] && fa[1] && fa[3]);
   auto fb = cat::make_simd_mask_ignore_last<float4x4>(1u);
   cat::verify(fb[0] && fb[2] && !fb[3]);
   auto fc = cat::make_simd_mask_keep_between<float4x4>(1u, 3u);
   cat::verify(!fc[0] && fc[1] && fc[2] && !fc[3]);
}

test(simd_mask_compose_and_if_else) {
   int4x4 v = {1, 2, 3, 4};
   auto m = cat::make_simd_mask_ignore_first<int4x4>(1u)
            & cat::make_simd_mask_from_count<int4x4>(3u);
   int4x4 r = cat::simd_if_else(m, v * 10, v);
   cat::verify(r[0] == 1);
   cat::verify(r[1] == 20);
   cat::verify(r[2] == 30);
   cat::verify(r[3] == 4);

   float4x4 fv = {1_f4, 2_f4, 3_f4, 4_f4};
   auto fm = cat::make_simd_mask_ignore_first<float4x4>(1u)
             & cat::make_simd_mask_from_count<float4x4>(3u);
   float4x4 fr = cat::simd_if_else(fm, fv * 10_f4, fv);
   cat::verify(fr[0] == 1_f4);
   cat::verify(fr[1] == 20_f4);
   cat::verify(fr[2] == 30_f4);
   cat::verify(fr[3] == 4_f4);
}

test(simd_if_else_builder_vs_factory) {
   int4x4 v = {1, 2, 3, 4};
   auto m = cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 factory = cat::simd_if_else(m, v + 100, 0);
   int4x4 r = cat::simd_if_else(cat::make_simd_mask_if(m).else_(0), v + 100);
   cat::verify(r[0] == factory[0]);
   cat::verify(r[3] == factory[3]);

   float4x4 fv = {1_f4, 2_f4, 3_f4, 4_f4};
   auto fm = cat::make_simd_mask_from_count<float4x4>(2u);
   float4x4 ffactory = cat::simd_if_else(fm, fv + 100_f4, 0_f4);
   float4x4 fr =
      cat::simd_if_else(cat::make_simd_mask_if(fm).else_(0_f4), fv + 100_f4);
   cat::verify(fr[0] == ffactory[0]);
   cat::verify(fr[3] == ffactory[3]);
}

// This feature is disabled for now.
// test(simd_tuple_protocol) {
//    static_assert(std::tuple_size_v<int4x4> == 4);
//    static_assert(cat::is_same<std::tuple_element_t<0u, int4x4>, cat::int4>);

//    using mask4 = int4x4::mask_type;
//    static_assert(std::tuple_size_v<mask4> == 4);
//    static_assert(
//       cat::is_same<std::tuple_element_t<0u, mask4>, mask4::lane_scalar>);

//    int4x4 v = {10, 20, 30, 40};
//    cat::verify(cat::get<0u>(v) == 10);
//    v.set_lane(1u, 99);
//    cat::verify(v[1u] == 99);
//    cat::get<1>(v) = 99;

//    auto const [a0, a1, a2, a3] = v;
//    cat::verify(a0 == 10);
//    cat::verify(a3 == 40);

//    mask4 m{};
//    m.fill(true);
//    m.set_lane(2u, false);
//    cat::verify(cat::get<0u>(m) != 0u);
//    cat::verify(!m[2u]);
//    cat::verify(m[0u]);

//    auto const [b0, b1, b2, b3] = m;
//    cat::verify(b0 != 0u && b1 != 0u && b2 == 0u && b3 != 0u);
// }

test(simd_iota) {
   int4x4 const from_iota_free = cat::iota<int4x4>(10);
   int4x4 const from_simd_iota = cat::simd_iota<int4x4>(10);
   cat::verify(from_iota_free[0] == 10);
   cat::verify(from_iota_free[3] == 13);
   cat::verify(from_simd_iota[1] == 11);
   cat::verify(from_simd_iota[2] == 12);

   float4x4 const float_from_iota = cat::iota<float4x4>(-1_f4);
   float4x4 const float_from_simd_iota = cat::simd_iota<float4x4>(-1_f4);
   cat::verify(float_from_iota[0] == -1_f4);
   cat::verify(float_from_iota[3] == 2_f4);
   cat::verify(float_from_simd_iota[2] == 1_f4);

   int4x4 const iota_passthrough_background = {100, 200, 300, 400};
   auto const iota_first_two_lanes_mask =
      cat::make_simd_mask_from_count<int4x4>(2u);
   int4x4 const int_masked_iota =
      cat::simd_iota<int4x4>[iota_first_two_lanes_mask](
         iota_passthrough_background, 10);
   cat::verify(int_masked_iota[0] == 10);
   cat::verify(int_masked_iota[1] == 11);
   cat::verify(int_masked_iota[2] == 300);
   cat::verify(int_masked_iota[3] == 400);

   float4x4 const float_iota_background = {10_f4, 20_f4, 30_f4, 40_f4};
   auto const float_iota_high_two_lanes_mask =
      cat::make_simd_mask_ignore_first<float4x4>(2u);
   float4x4 const float_masked_iota =
      cat::simd_iota<float4x4>[float_iota_high_two_lanes_mask](
         float_iota_background, 0);
   cat::verify(float_masked_iota[0] == 10_f4);
   cat::verify(float_masked_iota[1] == 20_f4);
   cat::verify(float_masked_iota[2] == 2_f4);
   cat::verify(float_masked_iota[3] == 3_f4);
}

test(simd_sub_sat_masked_native_int1) {
   // TODO: Something in here isn't vectorizing, and we get warnings.
   cat::int1x_ a{};
   cat::int1x_ b{};
   constexpr cat::idx n = cat::int1x_::abi_type::lanes;
   for (cat::idx i = 0; i < n; ++i) {
      a.set_lane(i, 10);
      b.set_lane(i, 20);
   }
   a.set_lane(0u, 100);
   b.set_lane(0u, 50);
   a.set_lane(1u, -40);
   b.set_lane(1u, 10);
   a.set_lane(2u, 120);
   b.set_lane(2u, -20);
   auto const m = a >= 0;
   cat::int1x_ const r = cat::simd_sub_sat[m](a, b);
   cat::verify(r[0] == 50);
   cat::verify(r[1] == -40);
   cat::verify(r[2] == 127);
   cat::int1x_ const add_r = cat::simd_add_sat[m](a, b);
   cat::verify(add_r[0] == 127);
   cat::verify(add_r[1] == -40);
   cat::verify(add_r[2] == 100);
}

test(simd_overflow_accessor_views) {
   using sat4 = cat::sat_int4;
   using sat4x4 = cat::fixed_size_simd<sat4, 4u>;

   sat4 const lane_max = cat::sat_int4::max();
   sat4x4 vmax{};
   for (cat::idx i = 0u; i < 4u; ++i) {
      vmax.set_lane(i, lane_max);
   }

   cat::verify((vmax + 1)[0] == lane_max);
   cat::verify((vmax.sat() + 1)[0] == lane_max);

   sat4 const lane_wrap =
      sat4(cat::wrap_add(lane_max.raw, typename sat4::raw_type(1)));
   cat::verify((vmax.undef() + 1)[0] == lane_wrap);
   cat::verify((vmax.wrap() + 1)[0] == lane_wrap);

   cat::int4 const scalar_max = cat::int4::max();
   cat::int4x4 u{};
   for (cat::idx i = 0u; i < 4u; ++i) {
      u.set_lane(i, scalar_max);
   }
   cat::int4x4 const bump_i(cat::int4(1));

   cat::verify((u.sat() + bump_i)[0] == scalar_max);

   cat::int4 const lane_expected =
      cat::int4(cat::wrap_add(scalar_max.raw, typename cat::int4::raw_type(1)));
   cat::verify((u + bump_i)[0] == lane_expected);
   cat::verify((u.undef() + bump_i)[0] == lane_expected);
   cat::verify((u.wrap() + bump_i)[0] == lane_expected);
}

test(simd_bool_lanes) {
   using simd_four_bool = cat::fixed_size_simd<bool, 4u>;

   simd_four_bool const f(true, false, true, false);
   simd_four_bool const g(false, false, true, true);

   cat::verify(f[0u] && !f[1u] && f[2u] && !f[3u]);
   cat::verify(!g[0u] && !g[1u] && g[2u] && g[3u]);

   cat::verify(f == f);
   cat::verify(!(f == g));

   auto const lane_eq = f.equal_lanes(g);
   cat::verify(!lane_eq[0u] && lane_eq[1u] && lane_eq[2u] && !lane_eq[3u]);

   simd_four_bool const ba = f & g;
   cat::verify(!ba[0u] && !ba[1u] && ba[2u] && !ba[3u]);

   simd_four_bool const bo = f | g;
   cat::verify(bo[0u] && !bo[1u] && bo[2u] && bo[3u]);

   simd_four_bool const bx = f ^ g;
   cat::verify(bx[0u] && !bx[1u] && !bx[2u] && bx[3u]);

   simd_four_bool splat(false);
   cat::verify(!(splat == true));
   splat.fill(true);
   cat::verify(splat == true);

   idx iter_n = 0u;
   for (bool lane : f) {
      cat::verify(lane == f[iter_n]);
      ++iter_n;
   }
   cat::verify(iter_n == f.size());

   cat::uint4x4 const pop_lane(15_u4);
   cat::verify(pop_lane.popcount()[0u] == 4_u4);

   cat::int4x4 const reduce_lane(3_i4);
   cat::verify(reduce_lane.reduce_add()[0u] == 12_i4);

   cat::sat_int4 const lane_max = cat::sat_int4::max();
   cat::fixed_size_simd<cat::sat_int4, 4u> vmax{};
   vmax.set_lane(0u, lane_max);
   cat::fixed_size_simd<cat::sat_int4, 4u> const bump(cat::sat_int4(1));
   cat::verify((vmax.sat() + bump)[0u] == lane_max);
}

test(simd_bool2_bool4_lanes_match_bool) {
   using simd_four_bool = cat::fixed_size_simd<bool, 4u>;
   using simd_four_bool2 = cat::fixed_size_simd<cat::bool2, 4u>;
   using simd_four_bool4 = cat::fixed_size_simd<cat::bool4, 4u>;

   simd_four_bool const f(true, false, true, false);
   simd_four_bool2 const f2(cat::bool2(true), cat::bool2(false),
                            cat::bool2(true), cat::bool2(false));
   simd_four_bool4 const f4(cat::bool4(true), cat::bool4(false),
                            cat::bool4(true), cat::bool4(false));

   verify_widened_bool_simd_matches_bool4(f, f2);
   verify_widened_bool_simd_matches_bool4(f, f4);

   simd_four_bool const g(false, false, true, true);
   simd_four_bool2 const g2(cat::bool2(false), cat::bool2(false),
                            cat::bool2(true), cat::bool2(true));
   simd_four_bool4 const g4(cat::bool4(false), cat::bool4(false),
                            cat::bool4(true), cat::bool4(true));

   verify_widened_bool_simd_matches_bool4(g, g2);
   verify_widened_bool_simd_matches_bool4(g, g4);

   cat::verify(f == f);
   cat::verify(f2 == f2);
   cat::verify(f4 == f4);
   cat::verify(!(f == g));

   simd_four_bool const lane_and = f & g;
   simd_four_bool2 const lane_and2 = f2 & g2;
   simd_four_bool4 const lane_and4 = f4 & g4;
   verify_widened_bool_simd_matches_bool4(lane_and, lane_and2);
   verify_widened_bool_simd_matches_bool4(lane_and, lane_and4);

   simd_four_bool const lane_or = f | g;
   simd_four_bool2 const lane_or2 = f2 | g2;
   simd_four_bool4 const lane_or4 = f4 | g4;
   verify_widened_bool_simd_matches_bool4(lane_or, lane_or2);
   verify_widened_bool_simd_matches_bool4(lane_or, lane_or4);

   simd_four_bool const lane_xor = f ^ g;
   simd_four_bool2 const lane_xor2 = f2 ^ g2;
   simd_four_bool4 const lane_xor4 = f4 ^ g4;
   verify_widened_bool_simd_matches_bool4(lane_xor, lane_xor2);
   verify_widened_bool_simd_matches_bool4(lane_xor, lane_xor4);

   simd_four_bool2 splat2(false);
   simd_four_bool4 splat4(false);
   simd_four_bool splat(false);
   verify_widened_bool_simd_matches_bool4(splat, splat2);
   verify_widened_bool_simd_matches_bool4(splat, splat4);
   cat::verify(!(splat == true));

   splat.fill(false);
   splat2.fill(cat::bool2(false));
   splat4.fill(cat::bool4(false));
   verify_widened_bool_simd_matches_bool4(splat, splat2);
   verify_widened_bool_simd_matches_bool4(splat, splat4);

   splat.fill(true);
   splat2.fill(cat::bool2(true));
   splat4.fill(cat::bool4(true));
   verify_widened_bool_simd_matches_bool4(splat, splat2);
   verify_widened_bool_simd_matches_bool4(splat, splat4);
   cat::verify(splat == true);
   cat::verify(splat4 == true);

   cat::simd_mask<bool, simd_four_bool::abi_type> const eq_ref =
      f.equal_lanes(g);
   cat::simd_mask<cat::bool2, simd_four_bool2::abi_type> const eq_2 =
      f2.equal_lanes(g2);
   cat::simd_mask<cat::bool4, simd_four_bool4::abi_type> const eq_4 =
      f4.equal_lanes(g4);

   for (cat::idx i = 0u; i < 4u; ++i) {
      cat::verify(eq_ref[i] == eq_2[i]);
      cat::verify(eq_ref[i] == eq_4[i]);
   }

   idx iter_b = 0u;
   for (bool lane : f) {
      cat::verify(lane == f[iter_b]);
      ++iter_b;
   }
   cat::verify(iter_b == f.size());

   idx iter_2 = 0u;
   for (cat::bool2 lane : f2) {
      cat::verify(lane == f[iter_2]);
      ++iter_2;
   }
   cat::verify(iter_2 == f2.size());

   idx iter_4 = 0u;
   for (cat::bool4 lane : f4) {
      cat::verify(lane == f[iter_4]);
      ++iter_4;
   }
   cat::verify(iter_4 == f4.size());
}
