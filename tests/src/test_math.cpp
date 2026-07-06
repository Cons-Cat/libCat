#include <cat/math>

#include "../unit_tests.hpp"
#include "cat/debug"

$test(math_min_max) {
   // Test `min()`.
   cat::verify(cat::min(0) == 0);
   cat::verify(cat::min(0u) == 0u);
   cat::verify(cat::min(0.f) == 0.f);
   cat::verify(cat::min(0.) == 0.);
   static_assert(cat::is_same<int, decltype(cat::min(0))>);
   static_assert(cat::is_same<unsigned, decltype(cat::min(0u))>);
   static_assert(cat::is_same<float, decltype(cat::min(0.f))>);
   static_assert(cat::is_same<double, decltype(cat::min(0.))>);

   cat::verify(cat::min(0, 1) == 0);
   cat::verify(cat::min(0u, 1u) == 0u);
   cat::verify(cat::min(0.f, 1.f) == 0.f);
   cat::verify(cat::min(0., 1.) == 0.);

   cat::verify(cat::min(0, 1, 2) == 0);
   cat::verify(cat::min(0u, 1u, 2u) == 0u);
   cat::verify(cat::min(0.f, 1.f, 2.f) == 0.f);
   cat::verify(cat::min(0., 1., 2.) == 0.);

   // Test `max()`.
   cat::verify(cat::max(0) == 0);
   cat::verify(cat::max(0u) == 0u);
   cat::verify(cat::max(0.f) == 0.f);
   cat::verify(cat::max(0.) == 0.);
   static_assert(cat::is_same<int, decltype(cat::max(0))>);
   static_assert(cat::is_same<unsigned, decltype(cat::max(0u))>);
   static_assert(cat::is_same<float, decltype(cat::max(0.f))>);
   static_assert(cat::is_same<double, decltype(cat::max(0.))>);

   cat::verify(cat::max(0, 1) == 1);
   cat::verify(cat::max(0u, 1u) == 1u);
   cat::verify(cat::max(0.f, 1.f) == 1.f);
   cat::verify(cat::max(0., 1.) == 1.);

   cat::verify(cat::max(0, 1, 2) == 2);
   cat::verify(cat::max(0u, 1u, 2u) == 2u);
   cat::verify(cat::max(0.f, 1.f, 2.f) == 2.f);
   cat::verify(cat::max(0., 1., 2.) == 2.);
}

$test(math_sum_product_dot) {
   cat::verify(cat::sum(2) == 2);
   cat::verify(cat::sum(2, 3, 4) == 9);
   static_assert(cat::is_same<int, decltype(cat::sum(2, 3, 4))>);

   cat::verify(cat::product(2) == 2);
   cat::verify(cat::product(2, 3, 4) == 24);
   static_assert(cat::is_same<int, decltype(cat::product(2, 3, 4))>);

   cat::verify(cat::dot(2, 3) == 6);
   cat::verify(cat::dot(2, 3, 4, 5) == 26);
}

$test(math_abs) {
   using namespace cat::arithmetic_literals;

   // Test `abs()`.
   cat::verify(cat::abs(1) == 1);
   cat::verify(cat::abs(1_i1) == 1_i1);
   cat::verify(cat::abs(1_i2) == 1_i2);
   cat::verify(cat::abs(1_i4) == 1_i4);
   cat::verify(cat::abs(1_i8) == 1_i8);

   cat::verify(cat::abs(1.f) == 1.f);
   cat::verify(cat::abs(1.) == 1.);
   cat::verify(cat::abs(-1) == 1);
   cat::verify(cat::abs(-1.5f) == 1.5f);
   cat::verify(cat::abs(-1.5) == 1.5);
   cat::verify(cat::abs(-1.5_f4) == 1.5_f4);
   cat::verify(cat::abs(-1.5_f8) == 1.5_f8);

   static_assert(cat::is_same<int, decltype(cat::abs(1))>);
   static_assert(cat::is_same<float, decltype(cat::abs(1.f))>);
   static_assert(cat::is_same<double, decltype(cat::abs(1.))>);

   cat::verify(cat::abs(1u) == 1);
   cat::verify(cat::abs(1_u1) == 1_u1);
   cat::verify(cat::abs(1_u2) == 1_u2);
   cat::verify(cat::abs(1_u4) == 1_u4);
   cat::verify(cat::abs(1_u8) == 1_u8);
}

$test(math_fma) {
   using namespace cat::arithmetic_literals;

   auto raw_result = cat::fma(2.f, 3.f, 4.f);
   static_assert(cat::is_same<decltype(raw_result), float4>);
   cat::verify(raw_result == 10_f4);

   auto raw8_result = cat::fma(2., 3., 4.);
   static_assert(cat::is_same<decltype(raw8_result), float8>);
   cat::verify(raw8_result == 10_f8);

   auto integer_result = cat::fma(5_i4, 3_i4, 2_i4);
   static_assert(cat::is_same<decltype(integer_result), int4>);
   cat::verify(integer_result == 17_i4);

   auto raw_integer_result = cat::fma(5, 3, 2);
   static_assert(cat::is_same<decltype(raw_integer_result), int>);
   cat::verify(raw_integer_result == 17);

   auto intptr_result = cat::fma(intptr<int>{5}, 3, 2);
   static_assert(cat::is_same<decltype(intptr_result), intptr<int>>);
   cat::verify(intptr_result == 17);

   auto float_result = cat::fma(2_f4, 3_f4, 4_f4);
   static_assert(cat::is_same<decltype(float_result), float4>);
   cat::verify(float_result == 10_f4);

   auto fast_float_result =
      cat::fma(float4_fast(2_f4), float4_fast(3_f4), float4_fast(4_f4));
   static_assert(cat::is_same<decltype(fast_float_result), float4_fast>);
   cat::verify(fast_float_result == 10_f4);

   auto float8_result = cat::fma(2_f8, 3_f8, 4_f8);
   static_assert(cat::is_same<decltype(float8_result), float8>);
   cat::verify(float8_result == 10_f8);

   auto fast_float8_result =
      cat::fma(float8_fast(2_f8), float8_fast(3_f8), float8_fast(4_f8));
   static_assert(cat::is_same<decltype(fast_float8_result), float8_fast>);
   cat::verify(fast_float8_result == 10_f8);
}

$test(math_pow) {
   // Test `pow()`.
   cat::verify(cat::pow(2, 2) == 4);
   static_assert(cat::is_same<int, decltype(cat::pow(2, 2))>);
   cat::verify(cat::pow(2, 1) == 2);
   cat::verify(cat::pow(2, 0) == 1);
   cat::verify(cat::pow(1, 10) == 1);
   cat::verify(cat::pow(8, -1) == 0);

   cat::verify(cat::pow(2u, 2u) == 4u);
   static_assert(cat::is_same<unsigned, decltype(cat::pow(2u, 2u))>);

   static_assert(cat::pow(wrap_uint1{cat::uint1_max}, 2) == wrap_uint1{1});
   static_assert(
      cat::pow(sat_uint1{cat::uint1_max}, 2) == cat::sat_uint1::max()
   );
   static_assert(cat::pow(sat_int1{cat::int1_max}, 2) == cat::sat_int1::max());

   cat::verify(cat::pow(2.f, 2) == 4.f);
   static_assert(cat::is_same<float, decltype(cat::pow(2.f, 2))>);
   cat::verify(cat::pow(2., 2) == 4.);
   static_assert(cat::is_same<double, decltype(cat::pow(2., 2))>);

   // cat::verify(cat::pow(2, 2) == 4.);
   // cat::verify(cat::pow(2, 1) == 2.);
   // cat::verify(cat::pow(2, 0) == 1.);
   // cat::verify(cat::pow(8, -1) == 0.);
}

$test(math_sqrt) {
   cat::verify(cat::sqrt(4.f) == 2.f);
   static_assert(cat::is_same<float, decltype(cat::sqrt(4.f))>);

   cat::verify(cat::sqrt(4.) == 2.);
   static_assert(cat::is_same<double, decltype(cat::sqrt(4.))>);
}

$test(math_rsqrt_cbrt_nroot) {
   auto near_one = [](float value) {
      float const diff = value > 1.f ? value - 1.f : 1.f - value;
      return diff < 1e-3f;
   };

   // `rsqrt(x) * sqrt(x) == 1`.
   cat::verify(near_one(cat::rsqrt(1.f) * cat::sqrt(1.f)));
   cat::verify(near_one(cat::rsqrt(4.f) * cat::sqrt(4.f)));
   cat::verify(near_one(cat::rsqrt(16.f) * cat::sqrt(16.f)));

   static_assert(cat::is_same<float, decltype(cat::rsqrt(4.f))>);
   static_assert(cat::is_same<double, decltype(cat::rsqrt(4.))>);

   // `fast`-policy `float` should route through the SSE `rsqrtss` + NR
   // path. The result must still be within ~1.5e-5 of the precise answer
   // (one NR step on a ~12-bit hardware estimate is comfortably below
   // single-precision tolerance).
   cat::float4_fast const fx = 4.f;
   cat::float4_fast const fast_r = cat::rsqrt(fx);
   cat::verify(near_one(fast_r.raw * 2.f));
   cat::verify(near_one(cat::rsqrt(cat::float4_fast(16.f)).raw * 4.f));

   // `cbrt`. Exact for perfect cubes within float precision tolerance.
   cat::verify(near_one(cat::cbrt(8.f) / 2.f));
   cat::verify(near_one(cat::cbrt(27.f) / 3.f));
   cat::verify(near_one(cat::cbrt(125.f) / 5.f));

   // Negative odd-`n` root keeps the sign.
   cat::verify(cat::cbrt(-8.f) < 0.f);
   cat::verify(near_one(-cat::cbrt(-8.f) / 2.f));

   // `nroot(x, n) ** n == x`. `pow` of `cbrt(64)` rebuilds 64.
   float const cbrt_64 = cat::cbrt(64.f);
   cat::verify(near_one(cbrt_64 * cbrt_64 * cbrt_64 / 64.f));

   // `nroot(_, 0)` and `nroot(_, 1)` are the identity.
   cat::verify(cat::nroot(7.f, 0) == 7.f);
   cat::verify(cat::nroot(7.f, 1) == 7.f);

   // `n == 4` round-trip.
   float const n4 = cat::nroot(81.f, 4);
   cat::verify(near_one(n4 / 3.f));

   // Even-`n` root of a negative is `NaN`.
   cat::verify(__builtin_isnan(cat::nroot(-16.f, 4)));
}

$test(math_rcbrt_rnroot) {
   auto near_one = [](float value) {
      float const diff = value > 1.f ? value - 1.f : 1.f - value;
      return diff < 1e-3f;
   };

   // `rcbrt(x) * cbrt(x) == 1`.
   cat::verify(near_one(cat::rcbrt(8.f) * cat::cbrt(8.f)));
   cat::verify(near_one(cat::rcbrt(27.f) * cat::cbrt(27.f)));

   // `rnroot(x, 4) * nroot(x, 4) == 1`.
   cat::verify(near_one(cat::rnroot(81.f, 4) * cat::nroot(81.f, 4)));
   cat::verify(near_one(cat::rnroot(256.f, 4) * cat::nroot(256.f, 4)));

   // `rnroot(_, 0)` is the identity (not the reciprocal).
   cat::verify(cat::rnroot(5.f, 0) == 5.f);

   // `rnroot(x, 1) == 1 / x`.
   cat::verify(cat::rnroot(4.f, 1) == 0.25f);
   cat::verify(cat::rnroot(2.f, 1) == 0.5f);
}

$test(math_div_floor) {
   using namespace cat::arithmetic_literals;

   static_assert(cat::div_floor(5, 2) == 2);
   static_assert(cat::div_floor(-5, 2) == -3);
   static_assert(cat::div_floor(5, -2) == -3);
   static_assert(cat::div_floor(-5, -2) == 2);

   cat::verify(cat::div_floor(5.f, 2.f) == 2.f);
   cat::verify(cat::div_floor(-5.f, 2.f) == -3.f);
   cat::verify(cat::div_floor(5.f, -2.f) == -3.f);
   cat::verify(cat::div_floor(-5.f, -2.f) == 2.f);
   cat::verify(cat::div_floor(5., 2.) == 2.);
   cat::verify(cat::div_floor(-5., 2.) == -3.);
   static_assert(cat::is_same<float, decltype(cat::div_floor(5.f, 2.f))>);
   static_assert(cat::is_same<double, decltype(cat::div_floor(5., 2.))>);

   // Precision policy on `T` is respected on the underlying divide.
   auto precise_result = cat::div_floor(5_f4, 2_f4);
   static_assert(cat::is_same<decltype(precise_result), cat::float4>);
   cat::verify(precise_result == 2_f4);

   auto fast_result =
      cat::div_floor(cat::float4_fast(5_f4), cat::float4_fast(2_f4));
   static_assert(cat::is_same<decltype(fast_result), cat::float4_fast>);
   cat::verify(fast_result == 2_f4);
}

$test(math_div_ceil) {
   using namespace cat::arithmetic_literals;

   static_assert(cat::div_ceil(5, 2) == 3);
   static_assert(cat::div_ceil(6, 2) == 3);
   static_assert(cat::div_ceil(7, 2) == 4);

   cat::verify(cat::div_ceil(5.f, 2.f) == 3.f);
   cat::verify(cat::div_ceil(-5.f, 2.f) == -2.f);
   cat::verify(cat::div_ceil(5.f, -2.f) == -2.f);
   cat::verify(cat::div_ceil(-5.f, -2.f) == 3.f);
   cat::verify(cat::div_ceil(5., 2.) == 3.);
   cat::verify(cat::div_ceil(-5., 2.) == -2.);
   static_assert(cat::is_same<float, decltype(cat::div_ceil(5.f, 2.f))>);
   static_assert(cat::is_same<double, decltype(cat::div_ceil(5., 2.))>);

   // Precision policy on `T` is respected on the underlying divide.
   auto precise_result = cat::div_ceil(5_f4, 2_f4);
   static_assert(cat::is_same<decltype(precise_result), cat::float4>);
   cat::verify(precise_result == 3_f4);

   auto fast_result =
      cat::div_ceil(cat::float4_fast(5_f4), cat::float4_fast(2_f4));
   static_assert(cat::is_same<decltype(fast_result), cat::float4_fast>);
   cat::verify(fast_result == 3_f4);
}

$test(math_round_to_multiple) {
   // Test `round_up_to_multiple_of()`.
   static_assert(cat::round_up_to_multiple_of(5u, 2u) == 6u);
   static_assert(cat::round_up_to_multiple_of(5u, 1u) == 5u);
   static_assert(cat::round_up_to_multiple_of(5u, 8u) == 8u);
   static_assert(
      cat::round_up_to_multiple_of(cat::idx(5), cat::idx(2)) == cat::idx(6)
   );
   static_assert(
      cat::round_up_to_multiple_of(cat::idx(5), cat::idx(3)) == cat::idx(6)
   );
   static_assert(
      cat::round_up_to_multiple_of(cat::idx(8), cat::idx(4)) == cat::idx(8)
   );
   // Test `round_down_to_multiple_of()`.
   static_assert(cat::round_down_to_multiple_of(5u, 2u) == 4u);
   static_assert(cat::round_down_to_multiple_of(5u, 1u) == 5u);
   static_assert(cat::round_down_to_multiple_of(5u, 8u) == 0u);
   static_assert(
      cat::round_down_to_multiple_of(cat::idx(5), cat::idx(2)) == cat::idx(4)
   );
   static_assert(
      cat::round_down_to_multiple_of(cat::idx(7), cat::idx(3)) == cat::idx(6)
   );
   static_assert(
      cat::round_down_to_multiple_of(cat::idx(8), cat::idx(4)) == cat::idx(8)
   );

   // `round_down_to_multiple_of()` floors signed values toward negative
   // infinity.
   static_assert(cat::round_down_to_multiple_of(-5, 2) == -6);
   static_assert(cat::round_down_to_multiple_of(-5, 8) == -8);
   static_assert(cat::round_down_to_multiple_of(-5, 3) == -6);
   static_assert(cat::round_down_to_multiple_of(-6, 3) == -6);
   static_assert(cat::round_down_to_multiple_of(-1, 4) == -4);
   static_assert(cat::round_down_to_multiple_of(3, 4) == 0);

   // `round_to_multiple_of()` picks the nearer multiple, with ties rounding
   // toward positive infinity, for both unsigned and signed inputs.
   static_assert(cat::round_to_multiple_of(5u, 2u) == 6u);
   static_assert(cat::round_to_multiple_of(4u, 3u) == 3u);
   static_assert(cat::round_to_multiple_of(5u, 3u) == 6u);
   static_assert(
      cat::round_to_multiple_of(cat::idx(4), cat::idx(3)) == cat::idx(3)
   );
   static_assert(cat::round_to_multiple_of(-4, 3) == -3);
   static_assert(cat::round_to_multiple_of(-5, 3) == -6);
   static_assert(cat::round_to_multiple_of(-5, 2) == -4);
   static_assert(cat::round_to_multiple_of(-6, 3) == -6);

   // When `multiple` is a wider type than `value`, the result widens to the
   // common type so it can hold a multiple that overflows `value`'s type.
   // `round_up_to_multiple_of(uint1{1}, uint2{256})` is `256`, which does not
   // fit in a `uint1`.
   static_assert(
      cat::is_same<
         decltype(cat::round_up_to_multiple_of(cat::uint1(1), cat::uint2(256))),
         cat::uint2>
   );
   static_assert(
      cat::round_up_to_multiple_of(cat::uint1(1), cat::uint2(256))
      == cat::uint2(256)
   );
   static_assert(
      cat::round_up_to_multiple_of(cat::uint1(1), cat::uint2(300))
      == cat::uint2(300)
   );
   static_assert(
      cat::round_down_to_multiple_of(cat::uint1(200), cat::uint2(256))
      == cat::uint2(0)
   );
   static_assert(cat::is_same<
                 decltype(cat::round_down_to_multiple_of(
                    cat::uint1(200), cat::uint2(256)
                 )),
                 cat::uint2>);
   static_assert(
      cat::round_to_multiple_of(cat::uint1(200), cat::uint2(256))
      == cat::uint2(256)
   );
}

$test(math_parity) {
   using namespace cat::arithmetic_literals;

   // Test `is_even()`.
   static_assert(cat::is_even(0));
   static_assert(cat::is_even(2));
   static_assert(cat::is_even(-4));
   static_assert(!cat::is_even(1));
   static_assert(!cat::is_even(-3));

   static_assert(cat::is_even(0u));
   static_assert(cat::is_even(2u));
   static_assert(!cat::is_even(1u));

   static_assert(cat::is_even(0_i1));
   static_assert(cat::is_even(2_i2));
   static_assert(cat::is_even(-4_i4));
   static_assert(cat::is_even(0_u4));
   static_assert(cat::is_even(2_u8));

   static_assert(!cat::is_even(1_i1));
   static_assert(!cat::is_even(-3_i4));
   static_assert(!cat::is_even(1_u4));

   static_assert(cat::is_same<bool, decltype(cat::is_even(0))>);
   static_assert(cat::is_same<bool, decltype(cat::is_even(0_i4))>);

   // Test `is_odd()`.
   static_assert(cat::is_odd(1));
   static_assert(cat::is_odd(-3));
   static_assert(!cat::is_odd(0));
   static_assert(!cat::is_odd(2));
   static_assert(!cat::is_odd(-4));

   static_assert(cat::is_odd(1u));
   static_assert(!cat::is_odd(0u));

   static_assert(cat::is_odd(1_i1));
   static_assert(cat::is_odd(-3_i4));
   static_assert(cat::is_odd(1_u4));
   static_assert(!cat::is_odd(0_i4));
   static_assert(!cat::is_odd(2_u8));

   static_assert(cat::is_same<bool, decltype(cat::is_odd(0))>);

   // `is_even` and `is_odd` partition the integers.
   static_assert(cat::is_even(7) != cat::is_odd(7));
   static_assert(cat::is_even(0) != cat::is_odd(0));
   static_assert(cat::is_even(-1) != cat::is_odd(-1));

   // Test `is_divisible_by()`.
   static_assert(cat::is_divisible_by(10, 5));
   static_assert(cat::is_divisible_by(10, 2));
   static_assert(cat::is_divisible_by(0, 5));
   static_assert(!cat::is_divisible_by(10, 3));
   static_assert(!cat::is_divisible_by(7, 4));

   static_assert(cat::is_divisible_by(16, 8));
   static_assert(cat::is_divisible_by(64u, 8u));
   static_assert(cat::is_divisible_by(1u, 1u));

   static_assert(cat::is_divisible_by(-10, 5));
   static_assert(cat::is_divisible_by(-10, -5));
   static_assert(!cat::is_divisible_by(-7, 4));

   // Only 0 is divisible by 0.
   static_assert(cat::is_divisible_by(0, 0));
   static_assert(!cat::is_divisible_by(1, 0));
   static_assert(!cat::is_divisible_by(-1, 0));

   // `T_min % -1` is undefined behaviour in C++ (the corresponding division
   // overflows). The signed-divisor `-1` short-circuit makes these well
   // defined. `cat::deconst()` inhibits the constant-folding `enable_if`
   // overload so the runtime short-circuit path is exercised.
   cat::verify(cat::is_divisible_by(cat::int1_min, cat::deconst(-1)));
   cat::verify(cat::is_divisible_by(cat::int2_min, cat::deconst(-1)));
   cat::verify(cat::is_divisible_by(cat::int4_min, cat::deconst(-1)));
   cat::verify(cat::is_divisible_by(cat::int8_min, cat::deconst(-1)));
   cat::verify(cat::is_divisible_by(0, cat::deconst(-1)));
   cat::verify(cat::is_divisible_by(7, cat::deconst(-1)));
   cat::verify(cat::is_divisible_by(-7, cat::deconst(-1)));

   static_assert(cat::is_same<bool, decltype(cat::is_divisible_by(10, 5))>);
}

$test(math_clamp) {
   // Test `clamp()`.
   cat::verify(cat::clamp(-10, 0, 10) == 0);
   cat::verify(cat::clamp(5, 0, 10) == 5);
   cat::verify(cat::clamp(20, 0, 10) == 10);

   cat::verify(cat::clamp(0u, 1u, 10u) == 1u);
   cat::verify(cat::clamp(5u, 1u, 10u) == 5u);
   cat::verify(cat::clamp(20u, 1u, 10u) == 10u);

   cat::verify(cat::clamp(-10.f, 0.f, 10.f) == 0.f);
   cat::verify(cat::clamp(5.f, 0.f, 10.f) == 5.f);
   cat::verify(cat::clamp(20.f, 0.f, 10.f) == 10.f);

   cat::verify(cat::clamp(-10., 0., 10.) == 0.);
   cat::verify(cat::clamp(5., 0., 10.) == 5.);
   cat::verify(cat::clamp(20., 0., 10.) == 10.);
}

$test(math_comparators) {
   // Comparators are inline constexpr niebloid instances. Calling with two
   // arguments performs the comparison directly.
   cat::verify(cat::is_equal_to(2, 2));
   cat::verify(!cat::is_equal_to(2, 3));

   cat::verify(cat::is_not_equal_to(2, 3));
   cat::verify(!cat::is_not_equal_to(2, 2));

   cat::verify(cat::is_less(1, 2));
   cat::verify(!cat::is_less(2, 1));

   cat::verify(cat::is_greater(2, 1));
   cat::verify(!cat::is_greater(1, 2));

   cat::verify(cat::is_less_equal(2, 2));
   cat::verify(cat::is_less_equal(1, 2));
   cat::verify(!cat::is_less_equal(2, 1));

   cat::verify(cat::is_greater_equal(2, 2));
   cat::verify(cat::is_greater_equal(2, 1));
   cat::verify(!cat::is_greater_equal(1, 2));

   // Mixed argument types compose directly (the underlying op deduces).
   cat::verify(cat::is_equal_to(1, 1l));
   cat::verify(cat::is_not_equal_to(1, 2l));
   cat::verify(cat::is_less(1, 2l));
   cat::verify(cat::is_greater(2, 1l));
   cat::verify(cat::is_less_equal(2, 2l));
   cat::verify(cat::is_greater_equal(2, 2l));

   cat::verify(cat::compare_three_way(1, 2) < 0);
   cat::verify(cat::compare_three_way(2, 2) == 0);
   cat::verify(cat::compare_three_way(3, 2) > 0);
}

$test(math_comparator_currying) {
   // One bound argument: `cmp(x)` returns a unary callable that compares
   // its argument against `x` (the bound left-hand side). Currying binds
   // the LEFT side, so `is_less(0)(y)` means `0 < y` (i.e. "y is positive").
   auto positive = cat::is_less(0);
   cat::verify(positive(1));
   cat::verify(!positive(0));
   cat::verify(!positive(-1));

   auto nonzero = cat::is_not_equal_to(0);
   cat::verify(nonzero(1));
   cat::verify(nonzero(-1));
   cat::verify(!nonzero(0));

   auto eq_3 = cat::is_equal_to(3);
   cat::verify(eq_3(3));
   cat::verify(eq_3(3l));
   cat::verify(!eq_3(4));

   // Currying preserves argument types: `cat::is_less(1)(2.5)` compares
   // `int` against `double` directly.
   auto greater_than_one = cat::is_less(1);
   cat::verify(greater_than_one(2));
   cat::verify(greater_than_one(2.5));
   cat::verify(!greater_than_one(1));

   auto at_most_five = cat::is_greater_equal(5);
   cat::verify(at_most_five(0));
   cat::verify(at_most_five(5));
   cat::verify(!at_most_five(6));

   auto at_least_five = cat::is_less_equal(5);
   cat::verify(at_least_five(5));
   cat::verify(at_least_five(6));
   cat::verify(!at_least_five(4));

   auto cmp_against_two = cat::compare_three_way(2);
   cat::verify(cmp_against_two(1) > 0);
   cat::verify(cmp_against_two(2) == 0);
   cat::verify(cmp_against_two(3) < 0);

   // The 0-bound niebloid and the 1-bound curried form both compose to the
   // same result.
   cat::verify(cat::is_less(1, 2));
   cat::verify(cat::is_less(1)(2));
}

$test(math_is_divisible_by_currying) {
   // Curried first argument fixes the dividend, so `is_divisible_by(12)(n)`
   // reads "is 12 divisible by n", i.e. "is n a divisor of 12".
   auto divisors_of_twelve = cat::is_divisible_by(12);
   cat::verify(divisors_of_twelve(1));
   cat::verify(divisors_of_twelve(2));
   cat::verify(divisors_of_twelve(3));
   cat::verify(divisors_of_twelve(4));
   cat::verify(divisors_of_twelve(6));
   cat::verify(divisors_of_twelve(12));
   cat::verify(!divisors_of_twelve(5));
   cat::verify(!divisors_of_twelve(7));

   // Signed `-1` short-circuit still applies through the curry.
   auto divisors_of_int4_min = cat::is_divisible_by(cat::int4_min);
   cat::verify(divisors_of_int4_min(cat::deconst(-1)));

   // Two-argument call still works on the unbound impl.
   cat::verify(cat::is_divisible_by(10, 5));
   cat::verify(!cat::is_divisible_by(10, 3));
}
