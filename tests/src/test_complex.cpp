#include <cat/detail/complex_builtins.hpp>

#include <cat/allocator_linear>
#include <cat/array>
#include <cat/complex>
#include <cat/format>
#include <cat/iterable>
#include <cat/math>
#include <cat/quantity>

#include "../unit_tests.hpp"

using namespace cat::arithmetic;
using namespace cat::units;

$test(complex_aliases) {
   static_assert(
      cat::is_same<cat::complex_int1::raw_type, _Complex signed char>
   );
   static_assert(sizeof(cat::complex_int1) == 2u * sizeof(signed char));

   static_assert(
      cat::is_same<cat::complex_uint1::raw_type, _Complex unsigned char>
   );
   static_assert(sizeof(cat::complex_uint1) == 2u * sizeof(unsigned char));

   static_assert(cat::is_same<cat::complex_int2::raw_type, _Complex short>);
   static_assert(sizeof(cat::complex_int2) == 2u * sizeof(short));

   static_assert(
      cat::is_same<cat::complex_uint2::raw_type, _Complex unsigned short>
   );
   static_assert(sizeof(cat::complex_uint2) == 2u * sizeof(unsigned short));

   static_assert(cat::is_same<cat::complex_int4::raw_type, _Complex int>);
   static_assert(sizeof(cat::complex_int4) == 2u * sizeof(int));

   static_assert(
      cat::is_same<cat::complex_uint4::raw_type, _Complex unsigned int>
   );
   static_assert(sizeof(cat::complex_uint4) == 2u * sizeof(unsigned int));

   static_assert(cat::is_same<cat::complex_int8::raw_type, _Complex long>);
   static_assert(sizeof(cat::complex_int8) == 2u * sizeof(long));

   static_assert(
      cat::is_same<cat::complex_uint8::raw_type, _Complex unsigned long>
   );
   static_assert(sizeof(cat::complex_uint8) == 2u * sizeof(unsigned long));

   static_assert(cat::is_same<cat::complex_float4::raw_type, _Complex float>);
   static_assert(sizeof(cat::complex_float4) == 2u * sizeof(float));

   static_assert(cat::is_same<cat::complex_float8::raw_type, _Complex double>);
   static_assert(sizeof(cat::complex_float8) == 2u * sizeof(double));

   // The aliases are visible through `cat::arithmetic` like `int4`.
   static_assert(
      cat::is_same<
         complex_int4,
         cat::basic_complex_int<int, cat::overflow_policies::undefined>>
   );
   static_assert(
      cat::is_same<complex_float8, cat::basic_complex_float<double>>
   );
   static_assert(
      cat::is_same<
         complex_float8,
         cat::basic_complex_float<double, cat::precision_policies::precise>>
   );
   static_assert(cat::is_same<complex_int1::value_type, int1>);
   static_assert(cat::is_same<complex_uint1::value_type, uint1>);

   static_assert(cat::is_same<complex_int2::value_type, int2>);
   static_assert(cat::is_same<complex_uint2::value_type, uint2>);

   static_assert(cat::is_same<complex_int4::value_type, int4>);
   static_assert(cat::is_same<complex_uint4::value_type, uint4>);

   static_assert(cat::is_same<complex_int8::value_type, int8>);
   static_assert(cat::is_same<complex_uint8::value_type, uint8>);

   static_assert(cat::is_same<complex_float4::value_type, float4>);
   static_assert(cat::is_same<complex_float8::value_type, float8>);
   static_assert(complex_int4::policy == cat::overflow_policies::undefined);
   static_assert(
      complex_float4::precision_policy == cat::precision_policies::precise
   );

   // Policies carry into the parts.
   static_assert(cat::is_same<wrap_complex_int4::value_type, cat::wrap_int4>);
   static_assert(cat::is_same<sat_complex_int1::value_type, cat::sat_int1>);
   static_assert(
      cat::is_same<complex_float8_fast::value_type, cat::float8_fast>
   );

   // There is no `_Complex __int128`, and components must match the template.
   // NOLINTNEXTLINE
   static_assert(requires { typename cat::basic_complex_int<__INT64_TYPE__>; });
   // NOLINTNEXTLINE
   static_assert(requires { typename cat::basic_complex_float<double>; });
   static_assert(!cat::detail::is_complex_int_component<__int128>);
   static_assert(!cat::detail::is_complex_int_component<unsigned __int128>);
   static_assert(!cat::detail::is_complex_int_component<float>);
   static_assert(!cat::detail::is_complex_int_component<bool>);
   static_assert(!cat::detail::is_complex_int_component<int4>);
   static_assert(!cat::detail::is_complex_float_component<int>);
   static_assert(!cat::detail::is_complex_float_component<long double>);
   static_assert(!cat::detail::is_complex_float_component<float4>);
}

$test(complex_trait) {
   static_assert(cat::is_complex<_Complex double>);
   static_assert(cat::is_complex<_Complex int const>);
   static_assert(cat::is_complex<complex_float8>);
   static_assert(cat::is_complex<sat_complex_int1 const>);
   static_assert(cat::is_complex<complex<metre_float8>>);
   static_assert(!cat::is_complex<double>);
   static_assert(!cat::is_complex<float8>);
   static_assert(!cat::is_complex<int4>);
   static_assert(!cat::is_complex<complex_float8&>);
}

$test(complex_construction_and_parts) {
   constexpr complex_float8 z{1.5 - 2.5i};
   static_assert(z.real() == 1.5);
   static_assert(z.imag() == -2.5);
   static_assert(cat::is_same<decltype(z.real()), float8>);

   constexpr complex_float8 real_only(float8(3.0));
   static_assert(real_only.real() == 3.0);
   static_assert(real_only.imag() == 0.0);

   constexpr complex_int4 integer{3 - 4i};
   static_assert(integer.real() == 3);
   static_assert(integer.imag() == -4);

   // Brace and implicit construction.
   constexpr complex_float4 braced{1.0f + 2.0fi};
   static_assert(braced.real() == 1.f && braced.imag() == 2.f);
   constexpr complex_float4 copied{3.0f + 4.0fi};
   static_assert(copied.real() == 3.f && copied.imag() == 4.f);

   // Deduction.
   static_assert(cat::is_same<
                 decltype(cat::basic_complex_float(float8(1.0), float8(2.0))),
                 complex_float8>);
   static_assert(
      cat::is_same<
         decltype(cat::basic_complex_int(int2(1), int2(2))), complex_int2>
   );

   // Conversions follow the rules of the parts. `float4` to `float8` is
   // explicit, as is narrowing.
   constexpr complex_float8 widened(complex_float4{1.0f + 2.0fi});
   static_assert(widened.real() == 1.0 && widened.imag() == 2.0);
   static_assert(!cat::is_convertible<complex_float4, complex_float8>);
   static_assert(!cat::is_convertible<complex_float8, complex_float4>);
   static_assert(cat::is_constructible<complex_float8, complex_float4>);
   static_assert(cat::is_constructible<complex_float4, complex_float8>);
   static_assert(cat::is_convertible<complex_int4, complex_int8>);
   constexpr complex_int8 widened_int = complex_int4{-7 + 9i};
   static_assert(widened_int.real() == -7 && widened_int.imag() == 9);
   static_assert(!cat::is_convertible<complex_uint4, complex_int4>);

   // A real number converts implicitly only when the part does. `int4` to
   // `float4` is explicit, so `int4` to `complex_float4` is too.
   static_assert(!cat::is_convertible<int4, float4>);
   static_assert(!cat::is_convertible<int4, complex_float4>);
   static_assert(cat::is_constructible<complex_float4, int4>);
   static_assert(!cat::is_convertible<float4, complex_float8>);
   static_assert(cat::is_constructible<complex_float8, float4>);
   static_assert(cat::is_convertible<float8, complex_float8>);
   static_assert(cat::is_convertible<double, complex_float8>);
   static_assert(cat::is_convertible<int4, complex_int8>);
   constexpr complex_float4 from_integer(int4(3));
   static_assert(from_integer.real() == 3.f && from_integer.imag() == 0.f);
   constexpr complex_float8 from_real = float8(2.5);
   static_assert(from_real.real() == 2.5 && from_real.imag() == 0.0);

   // Mutation of the parts.
   complex_float8 mutated{1.0 + 2.0i};
   mutated.real(5.0);
   mutated.imag(6.0);
   cat::verify(mutated.real() == 5.0);
   cat::verify(mutated.imag() == 6.0);

   // A raw `_Complex` converts implicitly.
   constexpr complex_float8 adopted = 7.0 + 8.0i;
   static_assert(adopted.real() == 7.0 && adopted.imag() == 8.0);
   static_assert(cat::is_convertible<_Complex double, complex_float8>);
   static_assert(cat::is_constructible<complex_float8, _Complex double>);
}

$test(complex_arithmetic) {
   constexpr complex_float8 a{1.0 + 2.0i};
   constexpr complex_float8 b{3.0 - 1.0i};

   static_assert(a + b == complex_float8{4.0 + 1.0i});
   static_assert(a - b == complex_float8{-2.0 + 3.0i});
   // (1 + 2i)(3 - i) = 3 - i + 6i + 2 = 5 + 5i
   static_assert(a * b == complex_float8{5.0 + 5.0i});
   // (5 + 5i) / (3 - i) = (5 + 5i)(3 + i) / 10 = (10 + 20i) / 10 = 1 + 2i
   static_assert((a * b) / b == a);
   static_assert(-a == complex_float8{-1.0 - 2.0i});
   static_assert(+a == a);
   static_assert(a != b);
   static_assert(a.conjugate() == complex_float8{1.0 - 2.0i});
   static_assert(cat::conj(a) == complex_float8{1.0 - 2.0i});
   static_assert(cat::is_same<decltype(cat::conj(a)), complex_float8>);
   static_assert(cat::conj(a).real() == 1.0);
   static_assert(cat::conj(a).imag() == -2.0);
   // (1 + 2i)(1 - 2i) = 5
   static_assert(a.norm() == 5.0);
   static_assert(a * a.conjugate() == complex_float8{5.0 + 0.0i});
   static_assert(cat::is_same<decltype(a + b), complex_float8>);

   // Scalars.
   static_assert(a + 1.0 == complex_float8{2.0 + 2.0i});
   static_assert(1.0 + a == complex_float8{2.0 + 2.0i});
   static_assert(a - float8(1.0) == complex_float8{0.0 + 2.0i});
   static_assert(a * 2.0 == complex_float8{2.0 + 4.0i});
   static_assert(2.0 * a == complex_float8{2.0 + 4.0i});
   static_assert(a / 2.0 == complex_float8{0.5 + 1.0i});
   static_assert(complex_float8{2.0 + 0.0i} / a == complex_float8{0.4 - 0.8i});
   static_assert(complex_float8{3.0 + 0.0i} == float8(3.0));
   static_assert(float8(3.0) == complex_float8{3.0 + 0.0i});
   static_assert(a != 1.0);

   // Compound assignment.
   complex_float8 c = a;
   c += b;
   cat::verify(c == complex_float8{4.0 + 1.0i});
   c -= b;
   cat::verify(c == a);
   c *= b;
   cat::verify(c == complex_float8{5.0 + 5.0i});
   c /= b;
   cat::verify(c == a);
   c *= 2.0;
   cat::verify(c == complex_float8{2.0 + 4.0i});
   c /= 2.0;
   cat::verify(c == a);

   // Mixed widths widen like `basic_float`.
   constexpr complex_float4 narrow{1.0f + 2.0fi};
   static_assert(cat::is_same<decltype(narrow + a), complex_float8>);
   static_assert(cat::is_same<decltype(a * narrow), complex_float8>);
   static_assert(cat::is_same<decltype(narrow + narrow), complex_float4>);
   static_assert(narrow + a == complex_float8{2.0 + 4.0i});
   // An integer scalar that fits in the mantissa widens a float.
   static_assert(cat::is_same<decltype(a + int4(1)), complex_float8>);
   static_assert(!cat::is_addable<complex_float4, int4>);
   static_assert(
      !cat::is_addable<complex_float4, complex_float8>
      || cat::is_same<
         decltype(cat::declval<complex_float4>() + cat::declval<complex_float8>()),
         complex_float8>
   );

   // Integers.
   constexpr complex_int4 x{1 + 2i};
   constexpr complex_int4 y{3 - 1i};
   static_assert(x + y == complex_int4{4 + 1i});
   static_assert(x - y == complex_int4{-2 + 3i});
   static_assert(x * y == complex_int4{5 + 5i});
   static_assert((x * y) / y == x);
   static_assert(-x == complex_int4{-1 - 2i});
   static_assert(x.conjugate() == complex_int4{1 - 2i});
   static_assert(cat::conj(x) == complex_int4{1 - 2i});
   static_assert(cat::is_same<decltype(cat::conj(x)), complex_int4>);
   static_assert(cat::conj(x).real() == 1);
   static_assert(cat::conj(x).imag() == -2);
   static_assert(x.norm() == 5);
   static_assert(x * 2 == complex_int4{2 + 4i});
   static_assert(x / 1 == x);
   static_assert(cat::is_same<decltype(x + y), complex_int4>);
   static_assert(
      complex_int1(100, 1) + complex_int1(20, 2) == complex_int1(120, 3)
   );
   // Mixed signedness has no lossless common type.
   static_assert(!cat::is_addable<complex_int4, complex_uint4>);
   static_assert(!cat::is_multipliable<complex_int4, complex_uint4>);
   static_assert(
      !cat::is_weakly_equality_comparable<complex_int4, complex_uint4>
   );
   static_assert(
      cat::is_same<
         decltype(complex_uint4{1u + 2ui} + complex_int8(1, 2)), complex_int8>
   );
   // A narrower unsigned operand converts to a wider signed one.
   static_assert(
      cat::is_same<
         decltype(complex_uint2(1u, 2u) + complex_int4{1 + 2i}), complex_int4>
   );

   // Every width round-trips.
   static_assert(
      complex_uint8(1u, 2u) * complex_uint8(3u, 4u)
      == complex_uint8(static_cast<unsigned long>(-5), 10u)
   );
   static_assert(
      complex_uint1(1u, 2u) + complex_uint1(1u, 2u) == complex_uint1(2u, 4u)
   );
   static_assert(
      complex_int2(1, 2) * complex_int2(3, 4) == complex_int2(-5, 10)
   );
}

$test(complex_runtime_arithmetic) {
   // The same arithmetic with values the compiler cannot fold.
   double const re = 1.0;
   double const im = 2.0;
   complex_float8 const a{float8(re), float8(im)};
   complex_float8 const b{3.0 - 1.0i};
   cat::verify(a * b == complex_float8{5.0 + 5.0i});
   cat::verify((a * b) / b == a);
   cat::verify(complex_float8{2.0 + 0.0i} / a == complex_float8{0.4 - 0.8i});

   int const integer_re = 1;
   int const integer_im = 2;
   complex_int4 const x{int4(integer_re), int4(integer_im)};
   cat::verify(x * complex_int4{3 - 1i} == complex_int4{5 + 5i});
   cat::verify(complex_int4{5 + 5i} / complex_int4{3 - 1i} == x);

   float const float_re = 1.f;
   complex_float4 const w{float4(float_re), 2.f};
   cat::verify(w * w == complex_float4{-3.f + 4.fi});
}

$test(complex_quantities) {
   constexpr complex<metre_float8> position(
      metre_float8(3.0), metre_float8(4.0)
   );
   static_assert(cat::is_same<complex<metre_float8>::value_type, metre_float8>);
   static_assert(
      cat::is_same<
         complex<metre_float8>::quantity_type, metre_float8::quantity_type>
   );
   static_assert(cat::has_quantity<complex<metre_float8>::value_type>);
   static_assert(position.real() == metre_float8(3.0));
   static_assert(position.imag() == metre_float8(4.0));
   static_assert(cat::is_same<decltype(position.real()), metre_float8>);

   // The quantity is preserved by addition and conjugation.
   static_assert(
      cat::is_same<decltype(position + position), complex<metre_float8>>
   );
   static_assert(cat::is_same<decltype(-position), complex<metre_float8>>);
   static_assert(
      cat::is_same<decltype(position.conjugate()), complex<metre_float8>>
   );
   static_assert(cat::conj(position).real() == metre_float8(3.0));
   static_assert(cat::conj(position).imag() == metre_float8(-4.0));
   static_assert(
      cat::is_same<decltype(cat::conj(position)), complex<metre_float8>>
   );
   static_assert((position + position).real() == metre_float8(6.0));
   static_assert(
      (position - position)
      == complex<metre_float8>(metre_float8(0.0), metre_float8(0.0))
   );

   // A number becomes a quantity with a dimension only explicitly, as in the
   // `std` quantities proposal. So `+`, `-`, `==`, `+=`, and `-=` reject it,
   // as they do for `metre_float8`. Scaling by a number is still valid.
   static_assert(!cat::is_addable<metre_float8, double>);
   static_assert(!cat::is_addable<complex<metre_float8>, double>);
   static_assert(!cat::is_addable<double, complex<metre_float8>>);
   static_assert(!cat::is_subtractable<complex<metre_float8>, double>);
   static_assert(!cat::is_subtractable<double, complex<metre_float8>>);
   static_assert(!cat::is_equality_comparable<complex<metre_float8>, double>);
   static_assert(!cat::is_add_assignable<metre_float8, double>);
   static_assert(!cat::is_add_assignable<complex<metre_float8>, double>);
   static_assert(!cat::is_subtract_assignable<complex<metre_float8>, double>);
   static_assert(!cat::is_convertible<double, complex<metre_float8>>);
   static_assert(
      position + complex<metre_float8>(1.0)
      == complex<metre_float8>(metre_float8(4.0), metre_float8(4.0))
   );
   // A dimensionless `basic_float` or complex number is the same as a number.
   static_assert(!cat::is_addable<metre_float8, float8>);
   static_assert(!cat::is_addable<complex<metre_float8>, float8>);
   static_assert(!cat::is_addable<complex<metre_float8>, complex_float8>);
   static_assert(!cat::is_convertible<complex_float8, complex<metre_float8>>);
   // Other quantities are still incompatible.
   static_assert(
      !cat::is_addable<complex<metre_float8>, complex<second_float8>>
   );
   static_assert(!cat::is_addable<complex<metre_float8>, second_float8>);
   static_assert(!cat::is_weakly_equality_comparable<
                 complex<metre_float8>, complex<second_float8>>);
   static_assert(
      !cat::is_weakly_equality_comparable<complex<metre_float8>, complex_float8>
   );
   static_assert(cat::is_addable<complex<metre_float8>, metre_float8>);
   static_assert(
      cat::is_same<
         decltype(position + metre_float8(1.0)), complex<metre_float8>>
   );
   static_assert(
      cat::is_same<
         decltype(metre_float8(1.0) + position), complex<metre_float8>>
   );
   static_assert((position + metre_float8(1.0)).real() == metre_float8(4.0));
   static_assert(
      position == complex<metre_float8>(metre_float8(3.0), metre_float8(4.0))
   );
   static_assert(
      position != complex<metre_float8>(metre_float8(3.0), metre_float8(5.0))
   );

   // Scaling by a dimensionless scalar keeps the quantity.
   static_assert(cat::is_same<decltype(position * 2.0), complex<metre_float8>>);
   static_assert(cat::is_same<decltype(2.0 * position), complex<metre_float8>>);
   static_assert(cat::is_same<decltype(position / 2.0), complex<metre_float8>>);
   static_assert((position * 2.0).real() == metre_float8(6.0));
   static_assert((position / 2.0).imag() == metre_float8(2.0));

   // Multiplying complex numbers multiplies their quantities.
   constexpr complex<second_float8> duration(
      second_float8(2.0), second_float8(0.0)
   );
   constexpr auto area = position * position;
   static_assert(cat::is_same<
                 decltype(area)::value_type,
                 decltype(metre_float8(1.0) * metre_float8(1.0))>);
   // (3 + 4i)^2 = 9 - 16 + 24i
   static_assert(area.real() == -7.0 * (metre * metre));
   static_assert(area.imag() == 24.0 * (metre * metre));
   static_assert(
      cat::is_same<
         decltype(area.real()), decltype(metre_float8(1.0) * metre_float8(1.0))>
   );

   constexpr auto metre_seconds = position * duration;
   static_assert(metre_seconds.real() == 6.0 * (metre * second));
   static_assert(metre_seconds.imag() == 8.0 * (metre * second));
   static_assert(cat::is_same<
                 decltype(metre_seconds.real()),
                 decltype(metre_float8(1.0) * second_float8(1.0))>);

   // Dividing divides quantities.
   constexpr auto velocity = position / duration;
   static_assert(velocity.real() == 1.5 * (metre / second));
   static_assert(velocity.imag() == 2.0 * (metre / second));
   static_assert(cat::is_same<
                 decltype(velocity.real()),
                 decltype(metre_float8(1.0) / second_float8(1.0))>);
   constexpr auto ratio = position / position;
   static_assert(ratio.real() == 1.0);
   static_assert(ratio.imag() == 0.0);
   static_assert(
      cat::is_same<decltype(ratio), complex_float8>
      || !cat::has_quantity<decltype(ratio)::value_type>
   );
   constexpr auto per_metre = 2.0 / position;
   static_assert(
      cat::is_same<
         decltype(per_metre.real()), decltype(1.0 / metre_float8(1.0))>
   );

   // The norm is the square of the quantity.
   static_assert(position.norm() == 25.0 * (metre * metre));
   static_assert(cat::is_same<
                 decltype(position.norm()),
                 decltype(metre_float8(1.0) * metre_float8(1.0))>);

   // Compound assignment cannot change the quantity.
   static_assert(
      !cat::is_multiply_assignable<complex<metre_float8>, complex<metre_float8>>
   );
   static_assert(cat::is_multiply_assignable<complex<metre_float8>, double>);
   static_assert(
      cat::is_add_assignable<complex<metre_float8>, complex<metre_float8>>
   );
   static_assert(
      !cat::is_add_assignable<complex<metre_float8>, complex_float8>
   );

   // Common units. Metres and nautical miles add in metres.
   constexpr complex<nautical_mile_float8> nautical(
      nautical_mile_float8(1.0), nautical_mile_float8(2.0)
   );
   constexpr auto sum = position + nautical;
   static_assert(sum.real() == 1'855.0 * metre);
   static_assert(sum.imag() == 3'708.0 * metre);
   static_assert(cat::is_same<decltype(sum.real()), metre_float8>);
   static_assert(
      nautical
      == complex<metre_float8>(metre_float8(1'852.0), metre_float8(3'704.0))
   );

   // Integers.
   constexpr complex<metre_int4> integer_position(metre_int4(3), metre_int4(4));
   static_assert(integer_position.real() == metre_int4(3));
   static_assert(
      cat::is_same<
         decltype(integer_position + integer_position), complex<metre_int4>>
   );
   static_assert((integer_position + integer_position).imag() == metre_int4(8));
   static_assert(!cat::is_addable<metre_int4, int4>);
   static_assert(!cat::is_addable<complex<metre_int4>, int4>);
   static_assert((integer_position * 2).real() == metre_int4(6));
   static_assert(
      cat::is_same<decltype(integer_position * 2), complex<metre_int4>>
   );
   static_assert(
      (integer_position * integer_position).real() == -7 * (metre * metre)
   );
   static_assert(
      (integer_position * integer_position).imag() == 24 * (metre * metre)
   );
   static_assert(integer_position.norm() == 25 * (metre * metre));
}

$test(complex_overflow_policies) {
   // `undefined` keeps the component width of its operands.
   static_assert(
      complex_int1(100, 1) + complex_int1(20, 2) == complex_int1(120, 3)
   );

   // `wrap` wraps each part.
   constexpr wrap_complex_uint1 wrapped =
      wrap_complex_uint1(cat::wrap_uint1(250u), cat::wrap_uint1(1u))
      + wrap_complex_uint1(cat::wrap_uint1(10u), cat::wrap_uint1(255u));
   static_assert(cat::is_same<decltype(wrapped), wrap_complex_uint1 const>);
   static_assert(wrapped.real() == 4u);
   static_assert(wrapped.imag() == 0u);
   constexpr wrap_complex_int4 big(
      cat::wrap_int4(2'147'483'647), cat::wrap_int4(0)
   );
   static_assert((big + big).real() == -2);
   static_assert((big * 2).real() == -2);
   static_assert(cat::is_same<decltype(big * 2), wrap_complex_int4>);

   // `saturate` saturates each part.
   constexpr sat_complex_int1 saturating(
      cat::sat_int1(100), cat::sat_int1(-100)
   );
   static_assert((saturating + saturating).real() == 127);
   static_assert((saturating + saturating).imag() == -128);
   static_assert(
      cat::is_same<decltype(saturating + saturating), sat_complex_int1>
   );
   // (100 - 100i)(100 - 100i) = 0 - 20000i. Only the exact parts saturate.
   static_assert((saturating * saturating).real() == 0);
   static_assert((saturating * saturating).imag() == -128);
   // (100 + 50i)(2 + 1i) = 150 + 200i. Saturating `100 * 2` first would make
   // the real part 127 - 50.
   constexpr sat_complex_int1 product =
      sat_complex_int1(cat::sat_int1(100), cat::sat_int1(50))
      * sat_complex_int1(cat::sat_int1(2), cat::sat_int1(1));
   static_assert(product.real() == 127);
   static_assert(product.imag() == 127);
   // The widest components still saturate the exact product.
   constexpr auto widest_unsigned = cat::limits<__UINT64_TYPE__>::max();
   constexpr sat_complex_uint8 widest{
      cat::sat_uint8(widest_unsigned), cat::sat_uint8(widest_unsigned)
   };
   static_assert((widest * widest).real() == 0u);
   static_assert((widest * widest).imag() == widest_unsigned);
   constexpr auto most_negative_signed = cat::limits<__INT64_TYPE__>::min();
   constexpr sat_complex_int8 most_negative{
      cat::sat_int8(most_negative_signed), cat::sat_int8(most_negative_signed)
   };
   static_assert((most_negative * most_negative).real() == 0);
   static_assert(
      (most_negative * most_negative).imag()
      == cat::limits<__INT64_TYPE__>::max()
   );
   // Division is exact before it saturates or wraps.
   static_assert(
      (sat_complex_int1(cat::sat_int1(100), cat::sat_int1(0))
       / sat_complex_int1(cat::sat_int1(2), cat::sat_int1(0)))
         .real()
      == 50
   );
   static_assert(
      (wrap_complex_int1(cat::wrap_int1(24), cat::wrap_int1(0))
       / wrap_complex_int1(cat::wrap_int1(12), cat::wrap_int1(0)))
         .real()
      == 2
   );
   // (100 + 100i) / (1 - 1i) = 100i, though `100 + 100` wraps.
   static_assert(
      (wrap_complex_int1(cat::wrap_int1(100), cat::wrap_int1(100))
       / wrap_complex_int1(cat::wrap_int1(1), cat::wrap_int1(-1)))
      == wrap_complex_int1(cat::wrap_int1(0), cat::wrap_int1(100))
   );
   static_assert(
      (sat_complex_uint1(cat::sat_uint1(1u), cat::sat_uint1(2u))
       - sat_complex_uint1(cat::sat_uint1(5u), cat::sat_uint1(1u)))
         .real()
      == 0u
   );
   static_assert(saturating.conjugate().imag() == 100);
   static_assert(cat::conj(saturating).imag() == 100);
   static_assert(cat::conj(saturating).real() == 100);
   static_assert(
      cat::is_same<decltype(cat::conj(saturating)), sat_complex_int1>
   );
   static_assert(saturating.norm() == 127);

   // Policies are left-associative.
   static_assert(
      cat::is_same<decltype(big + complex_int4{1 + 1i}), wrap_complex_int4>
   );
   static_assert(
      cat::is_same<decltype(complex_int4{1 + 1i} + big), complex_int4>
   );
   // A `wrap` or `saturate` left operand never widens, as `wrap_uint1 + uint2`
   // does not. The right operand is not narrowed first: the result is wrapped
   // or saturated once, from both operands' full values.
   static_assert(
      cat::is_same<
         decltype(wrap_complex_uint1{} + complex_uint2{}), wrap_complex_uint1>
   );
   static_assert(cat::is_addable<complex_uint1, complex_uint2>);
   static_assert(
      cat::is_same<decltype(sat_complex_int1{} * int4{}), sat_complex_int1>
   );
   constexpr sat_complex_int1 sat_one{cat::sat_int1(1), cat::sat_int1(-1)};
   static_assert((sat_one * int4(300)).real() == 127);
   static_assert((sat_one * int4(300)).imag() == -128);
   static_assert((sat_one - int4(300)).real() == -128);
   static_assert(
      (sat_one * complex_int4(int4(0), int4(300)))
      == sat_complex_int1(cat::sat_int1(127), cat::sat_int1(127))
   );
   constexpr wrap_complex_uint1 wrap_one{
      cat::wrap_uint1(1), cat::wrap_uint1(2)
   };
   static_assert((wrap_one + uint4(511)).real() == 0);
   static_assert((wrap_one * uint2(256)).imag() == 0);
   static_assert((wrap_one * uint2(257)).imag() == 2);

   // Runtime.
   __UINT8_TYPE__ const runtime_real = 250u;
   wrap_complex_uint1 const runtime_wrapped(
      cat::wrap_uint1(runtime_real), cat::wrap_uint1(0u)
   );
   cat::verify((runtime_wrapped + runtime_wrapped).real() == 244u);
   __INT8_TYPE__ const runtime_sat = 100;
   sat_complex_int1 const runtime_saturating(
      cat::sat_int1(runtime_sat), cat::sat_int1(0)
   );
   cat::verify((runtime_saturating * runtime_saturating).real() == 127);
   cat::verify(
      (runtime_saturating
       / sat_complex_int1(cat::sat_int1(2), cat::sat_int1(0)))
         .real()
      == 50
   );
}

$test(complex_precision_policies) {
   constexpr complex_float8_fast a(
      cat::float8_fast(1.0), cat::float8_fast(2.0)
   );
   constexpr complex_float8_fast b(
      cat::float8_fast(3.0), cat::float8_fast(-1.0)
   );
   static_assert(cat::is_same<decltype(a * b), complex_float8_fast>);
   static_assert(cat::is_same<decltype(a + 1.0), complex_float8_fast>);
   static_assert((a * b).real() == 5.0);
   static_assert((a * b).imag() == 5.0);
   static_assert(((a * b) / b).real() == 1.0);
   static_assert(a.norm() == 5.0);
   static_assert(cat::is_same<decltype(a.norm()), cat::float8_fast>);

   // Switching precision.
   static_assert(cat::conj(a).real() == 1.0);
   static_assert(cat::conj(a).imag() == -2.0);
   static_assert(cat::is_same<decltype(cat::conj(a)), complex_float8_fast>);
   static_assert(
      cat::is_same<
         decltype(a.precise()),
         cat::complex_precision_reference<
            complex_float8_fast const, cat::precision_policies::precise>>
   );
   static_assert(cat::is_same<decltype(a.precise() + a), complex_float8>);
   static_assert(
      cat::is_same<
         decltype(complex_float8{1.0 + 2.0i}.fast()), complex_float8_fast>
   );

   // Precision is left-associative.
   static_assert(
      cat::is_same<decltype(complex_float8{1.0 + 2.0i} + a), complex_float8>
   );
   static_assert(
      cat::is_same<
         decltype(a + complex_float8{1.0 + 2.0i}), complex_float8_fast>
   );
   static_assert(
      cat::is_same<
         decltype(complex_float4_fast(cat::float4_fast(1.f), cat::float4_fast(0.f)) * complex_float8{1.0 + 2.0i}),
         complex_float8_fast>
   );

   double const runtime_real = 1.0;
   complex_float8_fast const runtime{
      cat::float8_fast(runtime_real), cat::float8_fast(2.0)
   };
   cat::verify((runtime * b).real() == 5.0);
   cat::verify(((runtime * b) / b).imag() == 2.0);
}

$test(complex_alias_template) {
   static_assert(cat::is_same<cat::complex<int>, complex_int4>);
   static_assert(cat::is_same<cat::complex<unsigned>, complex_uint4>);

   static_assert(cat::is_same<cat::complex<signed char>, complex_int1>);
   static_assert(cat::is_same<cat::complex<unsigned char>, complex_uint1>);

   static_assert(cat::is_same<cat::complex<short>, complex_int2>);
   static_assert(cat::is_same<cat::complex<unsigned short>, complex_uint2>);

   static_assert(cat::is_same<cat::complex<long>, complex_int8>);
   static_assert(cat::is_same<cat::complex<unsigned long>, complex_uint8>);

   static_assert(cat::is_same<cat::complex<float>, complex_float4>);
   static_assert(cat::is_same<cat::complex<double>, complex_float8>);

   static_assert(cat::is_same<cat::complex<int4>, complex_int4>);
   static_assert(cat::is_same<cat::complex<float8>, complex_float8>);
   static_assert(cat::is_same<cat::complex<cat::wrap_int4>, wrap_complex_int4>);
   static_assert(
      cat::is_same<cat::complex<cat::float8_fast>, complex_float8_fast>
   );
   static_assert(
      cat::is_same<
         cat::complex<int, cat::overflow_policies::saturate>,
         cat::basic_complex_int<int, cat::overflow_policies::saturate>>
   );
   static_assert(cat::is_same<
                 cat::complex<float, cat::precision_policies::fast>,
                 complex_float4_fast>);
   static_assert(cat::is_same<cat::complex<metre_int4>, complex<metre_int4>>);
   static_assert(
      cat::is_same<cat::complex<metre_float8>, complex<metre_float8>>
   );
   static_assert(
      cat::is_same<cat::complex<metre_float8>::value_type, metre_float8>
   );
   static_assert(
      cat::complex<int>::policy == cat::overflow_policies::undefined
   );
   static_assert(
      cat::complex<double>::precision_policy == cat::precision_policies::precise
   );

   constexpr cat::complex<int> integer{1 + 2i};
   static_assert(integer + integer == complex_int4{2 + 4i});
   constexpr cat::complex<double> floating{1.0 + 2.0i};
   static_assert(cat::conj(floating) == complex_float8{1.0 - 2.0i});
   constexpr cat::complex<metre_float8> metres(
      metre_float8(3.0), metre_float8(4.0)
   );
   static_assert(metres.real() == metre_float8(3.0));
   static_assert(
      cat::is_same<decltype(metres + metres), complex<metre_float8>>
   );
}

$test(complex_tuple_protocol) {
   constexpr complex_float8 floating{1.5 - 2.5i};
   static_assert(std::tuple_size_v<complex_float8> == 2u);
   static_assert(std::tuple_size_v<complex_float8 const> == 2u);
   static_assert(cat::is_same<std::tuple_element_t<0, complex_float8>, float8>);
   static_assert(cat::is_same<std::tuple_element_t<1, complex_float8>, float8>);
   static_assert(
      cat::is_same<std::tuple_element_t<0, complex_float8 const>, float8 const>
   );
   static_assert(cat::get<0>(floating) == 1.5);
   static_assert(cat::get<1>(floating) == -2.5);
   static_assert(cat::is_same<decltype(cat::get<0>(floating)), float8 const&>);
   static_assert(
      cat::is_same<decltype(cat::get<0>(complex_float8{1.5 - 2.5i})), float8&&>
   );
   static_assert(cat::get<0>(complex_float8{1.5 - 2.5i}) == 1.5);
   static_assert(cat::get<1>(complex_float8{1.0 + 2.0i}) == 2.0);

   constexpr complex_float8 const frozen{1.5 - 2.5i};
   static_assert(
      cat::is_same<decltype(cat::get<1>(cat::move(frozen))), float8 const&&>
   );
   static_assert(cat::get<1>(cat::move(frozen)) == -2.5);

   complex_float8 mutable_float{1.0 + 2.0i};
   static_assert(cat::is_same<decltype(cat::get<0>(mutable_float)), float8&>);
   cat::get<0>(mutable_float) = float8(9.0);
   cat::get<1>(mutable_float) = float8(-3.0);
   cat::verify(mutable_float.real() == 9.0);
   cat::verify(mutable_float.imag() == -3.0);
   auto [copied_real, copied_imag] = mutable_float;
   cat::verify(copied_real == 9.0 && copied_imag == -3.0);
   auto& [real_part, imag_part] = mutable_float;
   real_part = float8(4.0);
   imag_part = float8(5.0);
   cat::verify(mutable_float.real() == 4.0);
   cat::verify(mutable_float.imag() == 5.0);
   float8 moved_real = cat::get<0>(cat::move(mutable_float));
   cat::verify(moved_real == 4.0);

   constexpr complex_int4 integer{3 + 4i};
   static_assert(std::tuple_size_v<complex_int4> == 2u);
   static_assert(cat::is_same<std::tuple_element_t<0, complex_int4>, int4>);
   static_assert(cat::is_same<std::tuple_element_t<1, complex_int4>, int4>);
   static_assert(cat::get<0>(integer) == 3);
   static_assert(cat::get<1>(integer) == 4);
   static_assert(
      cat::is_same<decltype(cat::get<0>(complex_int4{3 + 4i})), int4&&>
   );
   complex_int4 mutable_integer{1 + 2i};
   auto [int_real, int_imag] = mutable_integer;
   cat::verify(int_real == 1 && int_imag == 2);
   auto& [int_real_ref, int_imag_ref] = mutable_integer;
   int_real_ref = int4(8);
   int_imag_ref = int4(9);
   cat::verify(mutable_integer.real() == 8);
   cat::verify(mutable_integer.imag() == 9);
   int4 moved_integer = cat::get<0>(cat::move(mutable_integer));
   cat::verify(moved_integer == 8);

   constexpr complex<metre_float8> metres(metre_float8(3.0), metre_float8(4.0));
   static_assert(std::tuple_size_v<complex<metre_float8>> == 2u);
   static_assert(
      cat::is_same<std::tuple_element_t<0, complex<metre_float8>>, metre_float8>
   );
   static_assert(
      cat::is_same<std::tuple_element_t<1, complex<metre_float8>>, metre_float8>
   );
   static_assert(cat::get<0>(metres) == metre_float8(3.0));
   static_assert(cat::get<1>(metres) == metre_float8(4.0));
   complex<metre_float8> mutable_metres(metre_float8(3.0), metre_float8(4.0));
   auto& [metre_real, metre_imag] = mutable_metres;
   metre_real = metre_float8(6.0);
   cat::verify(mutable_metres.real() == metre_float8(6.0));
   cat::verify(metre_imag == metre_float8(4.0));
}

$test(complex_infinity) {
   constexpr complex_float8 infinite = cat::infinity;
   static_assert(infinite.real() == cat::infinity);
   static_assert(infinite.imag() == 0.0);
   constexpr complex_float8 negative_infinite(-cat::infinity);
   static_assert(negative_infinite.real() == -cat::infinity);
   constexpr complex_float8 both{cat::infinity, -cat::infinity};
   static_assert(both.real() == cat::infinity);
   static_assert(both.imag() == -cat::infinity);
   static_assert(cat::conj(both).real() == cat::infinity);
   static_assert(cat::conj(both).imag() == cat::infinity);

   constexpr auto over_zero = complex_float8{1.0 - 2.0i} / 0.0;
   static_assert(over_zero.real() == cat::infinity);
   static_assert(over_zero.imag() == -cat::infinity);
   constexpr auto over_negative_zero = complex_float8{1.0 - 2.0i} / -0.0;
   static_assert(over_negative_zero.real() == -cat::infinity);
   static_assert(over_negative_zero.imag() == cat::infinity);
   constexpr auto over_complex_zero =
      complex_float8{1.0 - 2.0i} / complex_float8{0.0 + 0.0i};
   static_assert(over_complex_zero.real() == cat::infinity);
   static_assert(over_complex_zero.imag() == -cat::infinity);
   // A negative zero divisor flips the infinities, as in `__divdc3`.
   constexpr auto over_negative_complex_zero =
      complex_float8{1.0 + 1.0i} / complex_float8(float8(-0.0), float8(0.0));
   static_assert(over_negative_complex_zero.real() == -cat::infinity);
   static_assert(over_negative_complex_zero.imag() == -cat::infinity);
   double const negative_zero = -0.0;
   auto const runtime_negative_complex_zero =
      cat::detail::complex_quotient(1.0, 1.0, negative_zero, 0.0);
   cat::verify(
      over_negative_complex_zero.real() == __real runtime_negative_complex_zero
   );
   cat::verify(
      over_negative_complex_zero.imag() == __imag runtime_negative_complex_zero
   );
   // A NaN part stays NaN over zero.
   constexpr auto nan_over_zero =
      complex_float8(float8(__builtin_nan("")), float8(1.0)) / 0.0;
   static_assert(nan_over_zero.real().is_nan());
   static_assert(nan_over_zero.imag() == cat::infinity);
   constexpr auto zero_over_zero = complex_float8{0.0 + 0.0i} / 0.0;
   static_assert(zero_over_zero.real().is_nan());
   static_assert(zero_over_zero.imag().is_nan());

   constexpr complex<metre_float8> infinite_metres{
      cat::infinity, -cat::infinity
   };
   // A quantity-bearing part converts from `cat::infinity` the same way
   // `metre_float8` does. `basic_float`'s `== 0` overload is ambiguous for
   // `infinity_type` when the quantity is not dimensionless, so compare the
   // component values.
   constexpr double positive_infinity = cat::infinity;
   static_assert(
      cat::make_raw_arithmetic(infinite_metres.real()) == positive_infinity
   );
   static_assert(
      cat::make_raw_arithmetic(infinite_metres.imag()) == -positive_infinity
   );
   static_assert(cat::is_same<decltype(infinite_metres.real()), metre_float8>);
   constexpr auto metre_over_zero =
      complex<metre_float8>(metre_float8(3.0), metre_float8(0.0)) / 0.0;
   static_assert(
      cat::make_raw_arithmetic(metre_over_zero.real()) == positive_infinity
   );
   static_assert(metre_over_zero.imag().is_nan());
   static_assert(cat::is_same<
                 decltype(metre_over_zero.real())::quantity_type,
                 decltype(metre_float8(1.0) / 1.0)::quantity_type>);
   static_assert(cat::is_same<
                 decltype(metre_over_zero.real())::raw_type,
                 decltype(metre_float8(1.0) / 1.0)::raw_type>);

   double const runtime_real = 1.0;
   complex_float8 const runtime(float8(runtime_real), float8(-2.0));
   auto const runtime_quotient = runtime / 0.0;
   cat::verify(runtime_quotient.real() == cat::infinity);
   cat::verify(runtime_quotient.imag() == -cat::infinity);
}

$test(complex_dot_and_inner_product) {
   // Hermitian: conj(1 + 2i) * (3 + 4i) = 11 - 2i.
   // Bilinear (1 + 2i) * (3 + 4i) is -5 + 10i, which is `inner_product`.
   static_assert(cat::dot(1.0 + 2.0i, 3.0 + 4.0i) == 11.0 - 2.0i);
   static_assert(cat::dot(1.0f + 2.0fi, 3.0f + 4.0fi) == 11.0f - 2.0fi);
   static_assert(
      cat::inner_product(0.0 + 0.0i, 1.0 + 2.0i, 3.0 + 4.0i) == -5.0 + 10.0i
   );
   static_assert(
      cat::inner_product(0.0f + 0.0fi, 1.0f + 2.0fi, 3.0f + 4.0fi)
      == -5.0f + 10.0fi
   );

   constexpr cat::complex<double> first{1.0 + 2.0i};
   constexpr cat::complex<double> second{3.0 + 4.0i};
   constexpr cat::complex<double> zero{0.0 + 0.0i};
   static_assert(cat::dot(first, second) == cat::complex<double>{11.0 - 2.0i});
   static_assert(
      cat::inner_product(zero, first, second)
      == cat::complex<double>{-5.0 + 10.0i}
   );
   static_assert(
      cat::is_same<decltype(cat::dot(first, second)), cat::complex<double>>
   );
   static_assert(cat::is_same<
                 decltype(cat::inner_product(zero, first, second)),
                 cat::complex<double>>);

   constexpr complex_int4 integer_left{1 + 2i};
   constexpr complex_int4 integer_right{3 + 4i};
   constexpr complex_int4 integer_zero{0};
   static_assert(
      cat::dot(integer_left, integer_right)
      == cat::conj(integer_left) * integer_right
   );
   static_assert(
      cat::inner_product(integer_zero, integer_left, integer_right)
      == integer_left * integer_right
   );
   static_assert(
      cat::is_same<
         decltype(cat::dot(integer_left, integer_right)), complex_int4>
   );

   constexpr auto paired =
      cat::dot(1.0 + 2.0i, 3.0 + 4.0i, 1.0 + 0.0i, 1.0 + 1.0i);
   static_assert(paired == 12.0 - 1.0i);

   constexpr complex<metre_float8> position(
      metre_float8(3.0), metre_float8(4.0)
   );
   constexpr auto norm = cat::dot(position, position);
   static_assert(cat::make_raw_arithmetic(norm.real()) == 25.0);
   static_assert(cat::make_raw_arithmetic(norm.imag()) == 0.0);
   static_assert(
      cat::is_same<
         decltype(norm.real()), decltype(metre_float8(1.0) * metre_float8(1.0))>
   );

   cat::array<complex_float8, 2u> left{1.0 + 2.0i, 1.0 + 0.0i};
   cat::array<complex_float8, 2u> right{3.0 + 4.0i, 1.0 + 1.0i};
   cat::verify(
      (left | cat::dot(right))
      == cat::conj(left[0u]) * right[0u] + cat::conj(left[1u]) * right[1u]
   );
   cat::verify(
      (left | cat::inner_product(right, complex_float8{0.0 + 0.0i}))
      == left[0u] * right[0u] + left[1u] * right[1u]
   );

   cat::array<int, 3u> values{1, 2, 3};
   cat::array<int, 3u> weights{4, 5, 6};
   cat::verify((values | cat::inner_product(weights, 0)) == 32);
   cat::verify(values.inner_product(weights, 10) == 42);

   constexpr auto add = [](auto left_value, auto right_value) {
      return left_value + right_value;
   };
   constexpr auto subtract = [](auto left_value, auto right_value) {
      return left_value - right_value;
   };
   // (1-4) + (2-5) + (3-6).
   cat::verify((values | cat::inner_product(weights, 0, add, subtract)) == -9);
}

$test(complex_format) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   auto allocator = cat::make_allocator_linear(page);

   cat::verify(
      cat::fmt(allocator, "{}", complex_int4{3 - 4i}).verify() == "3 - 4i"
   );

   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{}", complex_float8{1.5 + 2.5i}).verify()
      == "1.5E0 + 2.5E0i"
   );

   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{}", complex_float8{1.5 - 2.5i}).verify()
      == "1.5E0 - 2.5E0i"
   );

   allocator.reset();
   constexpr complex<metre_int4> steps(metre_int4(3), metre_int4(4));
   cat::verify(cat::fmt(allocator, "{}", steps).verify() == "3 + 4i m");

   allocator.reset();
   constexpr complex<metre_float8> position(
      metre_float8(3.0), metre_float8(4.0)
   );
   cat::verify(cat::fmt(allocator, "{}", position).verify() == "3E0 + 4E0i m");

   allocator.reset();
   constexpr wrap_complex_int4 wrapped(cat::wrap_int4(7), cat::wrap_int4(-2));
   cat::verify(cat::fmt(allocator, "{}", wrapped).verify() == "7 - 2i");

   // The most negative imaginary part has no positive counterpart.
   allocator.reset();
   constexpr complex_int1 most_negative(cat::int1(1), cat::int1(-128));
   cat::verify(cat::fmt(allocator, "{}", most_negative).verify() == "1 - 128i");

   allocator.reset();
   constexpr sat_complex_int1 saturated(cat::sat_int1(1), cat::sat_int1(-128));
   cat::verify(cat::fmt(allocator, "{}", saturated).verify() == "1 - 128i");
}

$test(complex_scale_binary) {
   // Below half the smallest subnormal rounds to zero.
   static_assert(cat::detail::scale_binary(0x1.000002p-126f, -25) == 0.f);
   static_assert(
      __builtin_signbit(cat::detail::scale_binary(-0x1.000002p-126f, -25))
   );
   static_assert(
      cat::detail::scale_binary(0x1.0000000000001p-1022, -54) == 0.0
   );
   // Exactly half rounds to even, which is zero.
   static_assert(cat::detail::scale_binary(0x1p-126f, -24) == 0.f);
   static_assert(cat::detail::scale_binary(0x1p-1022, -53) == 0.0);
   // Above half rounds up to the smallest subnormal.
   static_assert(cat::detail::scale_binary(0x1.000002p-126f, -24) == 0x1p-149f);
   static_assert(
      cat::detail::scale_binary(0x1.0000000000001p-1022, -53) == 0x1p-1074
   );
}

namespace {

template <typename T>
struct edge_case {
   T real;
   T imag;
};

// Every combination of signed zeros, small and large magnitudes, infinities,
// and NaNs, ending with a signaling NaN.
template <typename T>
consteval auto
make_edge_cases() -> cat::array<edge_case<T>, 154u> {
   T const nan = __builtin_nan("");
   T const infinity = __builtin_inf();
   cat::array<edge_case<T>, 154u> cases{};
   cat::idx count = 0u;
   auto const add = [&](T real, T imag) {
      cases[count] = {real, imag};
      ++count;
   };

   cat::array<T, 2u> const magnitudes{T(1.e-6), T(1.e+6)};
   cat::array<T, 2u> const zeros{T(-0.), T(0.)};

   for (T const real : magnitudes) {
      for (T const imag : magnitudes) {
         add(real, imag);
         add(-real, imag);
         add(-real, -imag);
         add(real, -imag);
      }
   }

   for (T const zero : zeros) {
      add(zero, T(-1.e-6));
      add(zero, T(1.e-6));
      add(zero, T(1.e+6));
      add(zero, T(-1.e+6));
   }

   for (T const zero : zeros) {
      add(T(-1.e-6), zero);
      add(T(1.e-6), zero);
      add(T(1.e+6), zero);
      add(T(-1.e+6), zero);
   }

   cat::array<T, 11u> const grid{
      nan,   -infinity, T(-2), T(-1), T(-0.5),  T(-0.),
      T(0.), T(0.5),    T(1),  T(2),  infinity,
   };
   for (T const imag : grid) {
      for (T const real : grid) {
         add(real, imag);
      }
   }

   add(infinity, __builtin_nans(""));
   return cases;
}

enum class edge : unsigned char {
   zero,
   non_zero,
   infinite,
   nan,
   non_zero_nan,
};

template <typename T>
constexpr auto
classify(T real, T imag) -> edge {
   if (real == T(0) && imag == T(0)) {
      return edge::zero;
   }

   if (__builtin_isinf(real) || __builtin_isinf(imag)) {
      return edge::infinite;
   }

   bool const real_nan = __builtin_isnan(real);
   bool const imag_nan = __builtin_isnan(imag);

   if (real_nan && imag_nan) {
      return edge::nan;
   }

   if (real_nan) {
      return imag == T(0) ? edge::nan : edge::non_zero_nan;
   }

   if (imag_nan) {
      return real == T(0) ? edge::nan : edge::non_zero_nan;
   }

   return edge::non_zero;
}

// The Annex G classification of `left * right`.
constexpr auto
expected_product(edge left, edge right) -> edge {
   switch (left) {
      case edge::zero:
         return (right == edge::zero || right == edge::non_zero) ? edge::zero
                                                                 : edge::nan;
      case edge::non_zero:
         return right == edge::non_zero_nan ? edge::nan : right;
      case edge::infinite:
         return (right == edge::zero || right == edge::nan) ? edge::nan
                                                            : edge::infinite;
      case edge::nan:
         return edge::nan;
      case edge::non_zero_nan:
         return right == edge::infinite ? edge::infinite : edge::nan;
   }
   __builtin_unreachable();
}

// The Annex G classification of `left / right`.
constexpr auto
expected_quotient(edge left, edge right) -> edge {
   switch (left) {
      case edge::zero:
         return (right == edge::non_zero || right == edge::infinite)
                   ? edge::zero
                   : edge::nan;
      case edge::non_zero:
         switch (right) {
            case edge::zero:
               return edge::infinite;
            case edge::non_zero:
               return edge::non_zero;
            case edge::infinite:
               return edge::zero;
            default:
               return edge::nan;
         }
      case edge::infinite:
         return (right == edge::zero || right == edge::non_zero)
                   ? edge::infinite
                   : edge::nan;
      case edge::nan:
         return edge::nan;
      case edge::non_zero_nan:
         return right == edge::zero ? edge::infinite : edge::nan;
   }
   __builtin_unreachable();
}

// Each part differs by less than `tolerance` times the larger magnitude.
template <typename T>
auto
is_complex_close(T real, T imag, T other_real, T other_imag, T tolerance)
   -> bool {
   T const magnitude = __builtin_fmax(
      __builtin_sqrt((real * real) + (imag * imag)),
      __builtin_sqrt((other_real * other_real) + (other_imag * other_imag))
   );
   return __builtin_fabs(real - other_real) < magnitude * tolerance
          && __builtin_fabs(imag - other_imag) < magnitude * tolerance;
}

// The Annex G helpers behind `__mulsc3`, `__muldc3`, `__divsc3`, and
// `__divdc3`.
template <typename T>
void
verify_builtin_edges() {
   constexpr auto cases = make_edge_cases<T>();
   T const product_tolerance = cat::is_same<T, double> ? T(1.e-9) : T(1.e-6);

   for (edge_case<T> const& left : cases) {
      edge const left_edge = classify(left.real, left.imag);
      for (edge_case<T> const& right : cases) {
         edge const right_edge = classify(right.real, right.imag);

         auto const product = cat::detail::complex_product(
            left.real, left.imag, right.real, right.imag
         );
         cat::verify(
            classify(__real product, __imag product)
            == expected_product(left_edge, right_edge)
         );

         auto const quotient = cat::detail::complex_quotient(
            left.real, left.imag, right.real, right.imag
         );
         cat::verify(
            classify(__real quotient, __imag quotient)
            == expected_quotient(left_edge, right_edge)
         );

         if (left_edge == edge::non_zero && right_edge == edge::non_zero) {
            cat::verify(is_complex_close(
               __real product, __imag product,
               (left.real * right.real) - (left.imag * right.imag),
               (left.real * right.imag) + (left.imag * right.real),
               product_tolerance
            ));
            T const denominator =
               (right.real * right.real) + (right.imag * right.imag);
            cat::verify(is_complex_close(
               __real quotient, __imag quotient,
               ((left.real * right.real) + (left.imag * right.imag))
                  / denominator,
               ((left.imag * right.real) - (left.real * right.imag))
                  / denominator,
               T(1.e-6)
            ));
         }
      }
   }
}

// `operator*` and `operator/` against the first `count` right operands for one
// left operand.
template <typename Complex>
constexpr auto
operator_edges_row(cat::idx row, cat::idx count) -> bool {
   using T = Complex::component_type;
   using part = Complex::value_type;
   constexpr auto cases = make_edge_cases<T>();
   Complex const left(part(cases[row].real), part(cases[row].imag));
   edge const left_edge = classify(cases[row].real, cases[row].imag);

   for (cat::idx column = 0u; column < count; ++column) {
      edge_case<T> const& right_case = cases[column];
      Complex const right(part(right_case.real), part(right_case.imag));
      edge const right_edge = classify(right_case.real, right_case.imag);

      Complex const product = left * right;
      if (
         classify(
            cat::make_raw_arithmetic(product.real()),
            cat::make_raw_arithmetic(product.imag())
         )
         != expected_product(left_edge, right_edge)
      ) {
         return false;
      }

      Complex const quotient = left / right;
      if (
         classify(
            cat::make_raw_arithmetic(quotient.real()),
            cat::make_raw_arithmetic(quotient.imag())
         )
         != expected_quotient(left_edge, right_edge)
      ) {
         return false;
      }
   }

   return true;
}

template <typename Complex>
auto
operator_edges() -> bool {
   for (cat::idx row = 0u; row < 154u; ++row) {
      if (!operator_edges_row<Complex>(row, 154u)) {
         return false;
      }
   }

   return true;
}

// Each row is its own constant evaluation, so each stays under Clang's step
// limit.
template <typename Complex, cat::idx row, cat::idx count>
constexpr bool is_constant_edge_row = operator_edges_row<Complex>(row, count);

template <typename Complex, cat::idx count, cat::idx... rows>
consteval auto
constant_operator_edges(cat::index_list_type<rows...>) -> bool {
   return (is_constant_edge_row<Complex, rows, count> && ...);
}

template <typename Complex>
auto
norm_edges() -> bool {
   using T = Complex::component_type;
   using part = Complex::value_type;
   constexpr auto cases = make_edge_cases<T>();

   for (edge_case<T> const& value : cases) {
      T const norm = cat::make_raw_arithmetic(
         Complex(part(value.real), part(value.imag)).norm()
      );
      switch (classify(value.real, value.imag)) {
         case edge::zero:
            if (norm != T(0) || __builtin_signbit(norm)) {
               return false;
            }
            break;
         case edge::non_zero:
            if (!__builtin_isfinite(norm) || !(norm > T(0))) {
               return false;
            }
            break;
         case edge::infinite:
            if (!__builtin_isinf(norm) || !(norm > T(0))) {
               return false;
            }
            break;
         case edge::nan:
         case edge::non_zero_nan:
            if (!__builtin_isnan(norm)) {
               return false;
            }
            break;
      }
   }

   return true;
}

// Converts to anything, which must not make the two-part constructor
// ambiguous.
struct anything_convertible {
   template <typename T>
   constexpr
   operator T() const {  // NOLINT
      return T(0);
   }
};

}  // namespace

$test(complex_annex_g_edges) {
   verify_builtin_edges<float>();
   verify_builtin_edges<double>();

   static_assert(
      constant_operator_edges<complex_float4, 154u>(cat::index_sequence<154u>)
   );
   // The last case has a signaling NaN part. Clang's constant evaluator takes
   // the divisor's larger magnitude with IEEE `maxNum`, which makes
   // `max(infinity, sNaN)` a NaN, so `(1 + 1i) / (infinity + sNaN i)` is a NaN
   // there and zero at runtime. Converting the case to `float` quiets the NaN,
   // so only `double` leaves it out.
   static_assert(
      constant_operator_edges<complex_float8, 153u>(cat::index_sequence<153u>)
   );
   cat::verify(operator_edges<complex_float4>());
   cat::verify(operator_edges<complex_float8>());

   // An infinite part makes the norm infinite, even with a NaN part.
   cat::verify(norm_edges<complex_float4>());
   cat::verify(norm_edges<complex_float8>());
}

$test(complex_layout_and_conversions) {
   // The real part is stored first.
   complex_float8 value{};
   auto* const parts = reinterpret_cast<double*>(&value);
   cat::verify(parts[0] == 0.0 && parts[1] == 0.0);
   parts[0] = 5.0;
   parts[1] = 6.0;
   cat::verify(value.real() == 5.0 && value.imag() == 6.0);

   static_assert(
      __builtin_bit_cast(complex_float4, cat::array<unsigned char, 8u>{}).real()
      == 0.f
   );
   static_assert(
      __builtin_bit_cast(complex_float8, cat::array<unsigned char, 16u>{})
         .imag()
      == 0.0
   );

   constexpr anything_convertible anything;
   constexpr complex_float8 from_anything(anything, anything);
   static_assert(from_anything.real() == 0.0 && from_anything.imag() == 0.0);

   // Compound assignment from another complex type.
   complex_float8 sum{3.0 + 5.0i};
   sum += complex_int2(int2(1), int2(1));
   cat::verify(sum == complex_float8{4.0 + 6.0i});
   sum -= complex_float4{1.f + 1.fi};
   cat::verify(sum == complex_float8{3.0 + 5.0i});
   complex_float8 product{-4.0 + 7.5i};
   product *= complex_int2(int2(1), int2(1));
   cat::verify(product == complex_float8{-11.5 + 3.5i});
   complex_float8 quotient{1.0 + 0.0i};
   quotient /= complex_float4{1.f + 1.fi};
   cat::verify(quotient == complex_float8{0.5 - 0.5i});
}

namespace {

[[nodiscard]]
constexpr auto
real_of(auto const& value) -> double {
   return cat::make_raw_arithmetic(value.real());
}

[[nodiscard]]
constexpr auto
imag_of(auto const& value) -> double {
   return cat::make_raw_arithmetic(value.imag());
}

[[nodiscard]]
auto
is_about(double value, double expected) -> bool {
   return __builtin_fabs((value - expected) / (value + expected)) < 1.e-14;
}

[[nodiscard]]
auto
is_near(double value, double expected, double tolerance) -> bool {
   double const scale = __builtin_fmax(1., __builtin_fabs(expected));
   return __builtin_fabs(value - expected) <= tolerance * scale;
}

[[nodiscard]]
constexpr auto
is_near_constant(double value, double expected) -> bool {
   double const difference = value - expected;
   return difference < 1.e-9 && difference > -1.e-9;
}

[[nodiscard]]
auto
has_sign(double value) -> bool {
   return __builtin_signbit(value) != 0;
}

// The same value, including the sign of a zero, or both NaN.
[[nodiscard]]
auto
is_identical(double value, double other) -> bool {
   if (__builtin_isnan(value) || __builtin_isnan(other)) {
      return __builtin_isnan(value) && __builtin_isnan(other);
   }

   return value == other && has_sign(value) == has_sign(other);
}

[[nodiscard]]
auto
make_case(edge_case<double> parts) -> complex_float8 {
   return complex_float8(float8(parts.real), float8(parts.imag));
}

[[nodiscard]]
auto
make_angle_case(edge_case<double> parts) -> complex<cat::units::radian_float8> {
   return complex<cat::units::radian_float8>(
      cat::units::radian_float8(parts.real),
      cat::units::radian_float8(parts.imag)
   );
}

constexpr auto math_cases = make_edge_cases<double>();

template <typename T>
concept has_complex_exp = requires(T value) { cat::exp(value); };

template <typename T>
concept has_complex_sin = requires(T value) { cat::sin(value); };

template <typename T>
concept has_complex_asin = requires(T value) { cat::asin(value); };

template <typename T, typename U>
concept has_complex_pow =
   requires(T base, U exponent) { cat::pow(base, exponent); };
constexpr double math_pi = cat::pi<double>;

}  // namespace

$test(complex_math_values) {
   // Magnitude and phase.
   cat::verify(cat::abs(complex_float8{3.0 + 4.0i}) == 5.0);
   static_assert(cat::is_same<decltype(cat::abs(complex_float8{})), float8>);
   static_assert(is_near_constant(
      cat::make_raw_arithmetic(cat::abs(complex_float8{3.0 + 4.0i})), 5.0
   ));
   constexpr complex<metre_float8> displacement(
      metre_float8(3.0), metre_float8(4.0)
   );
   static_assert(cat::is_same<decltype(cat::abs(displacement)), metre_float8>);
   cat::verify(cat::abs(displacement) == metre_float8(5.0));
   cat::verify(cat::arg(complex_float8{1.0 + 0.0i}) == 0.0);
   static_assert(
      cat::is_same<
         decltype(cat::arg(complex_float8{})), cat::units::radian_float8>
   );
   cat::verify(is_about(
      cat::make_raw_arithmetic(cat::arg(complex_float8{0.0 + 1.0i})),
      math_pi / 2
   ));

   // Exponentials and logarithms.
   cat::verify(cat::exp(complex_float8{0.0 + 0.0i}) == complex_float8{1.0});
   cat::verify(
      cat::log(complex_float8{0.0 + 0.0i})
      == complex_float8(float8(-cat::infinity), float8(0.0))
   );
   constexpr auto unit_circle = cat::exp(complex_float8{0.0 + 1.0i});
   static_assert(is_near_constant(real_of(unit_circle), 0.5403023058681398));
   static_assert(is_near_constant(imag_of(unit_circle), 0.8414709848078965));
   complex_float8 const logarithm = cat::log(complex_float8{-1.0 + 0.0i});
   cat::verify(
      real_of(logarithm) == 0.0 && is_about(imag_of(logarithm), math_pi)
   );
   // A raw `_Complex` works too, and produces a libCat complex.
   static_assert(cat::is_same<decltype(cat::exp(1.0 + 2.0i)), complex_float8>);
   static_assert(
      cat::is_same<decltype(cat::exp(complex_float4{})), complex_float4>
   );

   // Roots.
   cat::verify(cat::sqrt(complex_float8{64.0 + 0.0i}) == complex_float8{8.0});
   cat::verify(
      cat::sqrt(complex_float8{-4.0 + 0.0i}) == complex_float8{0.0 + 2.0i}
   );
   cat::verify(
      cat::sqrt(complex_float8(float8(-4.0), float8(-0.0)))
      == complex_float8{0.0 - 2.0i}
   );
   cat::verify(
      cat::sqrt(complex_float8{3.0 + 4.0i}) == complex_float8{2.0 + 1.0i}
   );
   complex_float8 const reciprocal_root =
      cat::rsqrt(complex_float8{-4.0 + 0.0i});
   cat::verify(real_of(reciprocal_root) == 0.0);
   cat::verify(imag_of(reciprocal_root) == -0.5);
   // The principal cube root of `-8` is `1 + sqrt(3) i`.
   complex_float8 const cube_root = cat::cbrt(complex_float8{-8.0 + 0.0i});
   cat::verify(is_near(real_of(cube_root), 1.0, 1.e-15));
   cat::verify(is_near(imag_of(cube_root), 1.7320508075688772, 1.e-15));
   cat::verify(cat::nroot(complex_float8{-8.0 + 0.0i}, 3) == cube_root);
   cat::verify(
      cat::nroot(complex_float8{-8.0 + 0.0i}, 1) == complex_float8{-8.0}
   );
   complex_float8 const reciprocal_cube_root =
      cat::rcbrt(complex_float8{-8.0 + 0.0i});
   cat::verify(is_near(real_of(reciprocal_cube_root), 0.25, 1.e-15));
   cat::verify(
      is_near(imag_of(reciprocal_cube_root), -0.4330127018922193, 1.e-15)
   );
   cat::verify(
      cat::rnroot(complex_float8{-8.0 + 0.0i}, 3) == reciprocal_cube_root
   );

   // Roots and powers raise the quantity.
   constexpr auto area = cat::pow<2>(displacement);
   static_assert(
      cat::is_same<
         decltype(area),
         complex<decltype(metre_float8(1.0) * metre_float8(1.0))> const>
   );
   static_assert(
      area.real() == -7.0 * (metre * metre)
      && area.imag() == 24.0 * (metre * metre)
   );
   auto const side = cat::sqrt(area);
   static_assert(cat::is_same<
                 decltype(side)::value_type,
                 decltype(cat::sqrt(metre_float8(1.0) * metre_float8(1.0)))>);
   cat::verify(is_near(real_of(side), 3.0, 1.e-15));
   cat::verify(is_near(imag_of(side), 4.0, 1.e-15));
   auto const per_side = cat::rsqrt(area);
   static_assert(
      cat::is_same<
         decltype(per_side), complex<decltype(1.0 / metre_float8(1.0))> const>
   );
   auto const edge = cat::nroot<2>(area);
   static_assert(cat::is_same<decltype(edge), decltype(side)>);
   static_assert(!has_complex_exp<complex<metre_float8>>);

   // Powers.
   cat::verify(
      cat::pow(complex_float8{2.0 + 3.0i}, 2) == complex_float8{-5.0 + 12.0i}
   );
   static_assert(
      cat::pow(complex_float8{2.0 + 3.0i}, 2) == complex_float8{-5.0 + 12.0i}
   );
   cat::verify(
      cat::pow(complex_float8{0.0 + 2.0i}, -1) == complex_float8{0.0 - 0.5i}
   );
   complex_float8 const complex_power =
      cat::pow(complex_float8{2.0 + 3.0i}, complex_float8{2.0 + 0.0i});
   cat::verify(is_about(real_of(complex_power), -5.0));
   cat::verify(is_about(imag_of(complex_power), 12.0));
   complex_float8 const real_power = cat::pow(complex_float8{2.0 + 3.0i}, 2.0);
   cat::verify(is_about(real_of(real_power), -5.0));
   cat::verify(is_about(imag_of(real_power), 12.0));
   complex_float8 const scalar_power = cat::pow(2.0, complex_float8{2.0});
   cat::verify(is_about(real_of(scalar_power), 4.0));
   cat::verify(__builtin_fabs(imag_of(scalar_power)) < 1.e-6);
   static_assert(
      cat::is_same<decltype(cat::pow(2.f, complex_float4{})), complex_float4>
   );
   static_assert(!has_complex_pow<float, complex_float8>);
   // Integer complex numbers have exact integer powers.
   static_assert(cat::pow(complex_int4{1 + 2i}, 3) == complex_int4{-11 - 2i});
   static_assert(cat::pow(complex_int4{0 + 1i}, -1) == complex_int4{0 - 1i});
   static_assert(cat::pow(complex_int4{3 + 4i}, 0u) == complex_int4{1 + 0i});
   static_assert(
      cat::pow(sat_complex_int1(cat::sat_int1(10), cat::sat_int1(0)), 3)
      == sat_complex_int1(cat::sat_int1(127), cat::sat_int1(0))
   );
   constexpr complex<metre_int4> integer_metres(metre_int4(1), metre_int4(2));
   static_assert(cat::pow<2>(integer_metres).real() == -3 * (metre * metre));
   static_assert(cat::pow<2>(integer_metres).imag() == 4 * (metre * metre));

   // Circular functions take angles, and inverse ones produce radians.
   using radian_complex = complex<cat::units::radian_float8>;
   radian_complex const zero_angle{};
   cat::verify(cat::sin(zero_angle) == complex_float8{});
   cat::verify(cat::cos(zero_angle) == complex_float8{1.0});
   cat::verify(cat::tan(zero_angle) == complex_float8{});
   static_assert(cat::is_same<decltype(cat::sin(zero_angle)), complex_float8>);
   cat::verify(
      cat::tan(radian_complex(
         cat::units::radian_float8(10'000.0),
         cat::units::radian_float8(-10'000.0)
      ))
      == complex_float8{0.0 - 1.0i}
   );
   static_assert(!has_complex_sin<complex_float8>);
   static_assert(!has_complex_sin<_Complex double>);
   static_assert(has_complex_sin<radian_complex>);
   constexpr complex<cat::units::degree_float8> right_angle(
      cat::units::degree_float8(90.0), cat::units::degree_float8(0.0)
   );
   static_assert(is_near_constant(real_of(cat::sin(right_angle)), 1.0));
   cat::verify(cat::asin(complex_float8{}) == radian_complex{});
   static_assert(
      cat::is_same<decltype(cat::asin(complex_float8{})), radian_complex>
   );
   cat::verify(cat::atan(complex_float8{}) == radian_complex{});
   radian_complex const infinite_arc =
      cat::acos(complex_float8(float8(cat::infinity), float8(1.0)));
   cat::verify(real_of(infinite_arc) == 0.0);
   cat::verify(imag_of(infinite_arc) == -cat::infinity);
   static_assert(!has_complex_asin<complex<metre_float8>>);

   // Hyperbolic functions.
   cat::verify(cat::sinh(complex_float8{}) == complex_float8{});
   cat::verify(cat::cosh(complex_float8{}) == complex_float8{1.0});
   cat::verify(cat::tanh(complex_float8{}) == complex_float8{});
   constexpr auto hyperbolic = cat::sinh(complex_float8{1.0 + 1.0i});
   static_assert(is_near_constant(real_of(hyperbolic), 0.6349639147847361));
   static_assert(is_near_constant(imag_of(hyperbolic), 1.2984575814159773));

   // Multiply-add, sum, and product mix complex and real operands.
   static_assert(
      cat::fma(complex_float8{1.0 + 2.0i}, complex_float8{3.0 + 4.0i}, 1.0)
      == complex_float8{-4.0 + 10.0i}
   );
   static_assert(
      cat::fma(2.0, complex_float8{1.0 + 1.0i}, complex_float8{0.0 + 1.0i})
      == complex_float8{2.0 + 3.0i}
   );
   static_assert(
      cat::sum(complex_float8{1.0 + 2.0i}, 3.0, complex_float8{0.0 - 1.0i})
      == complex_float8{4.0 + 1.0i}
   );
   static_assert(
      cat::product(complex_int4{0 + 1i}, complex_int4{0 + 1i}, 3)
      == complex_int4{-3 + 0i}
   );
}

$test(complex_math_lgamma) {
   // Reference values come from shifting far into Stirling's range.
   struct reference {
      double real;
      double imag;
      double expected_real;
      double expected_imag;
   };

   cat::array<reference, 9u> const references{
      reference{0.0,    1.0,   -0.6509231993018432, -1.8724366472624272},
      reference{3.0,    4.0,   -1.7566267846036965, 4.742664438034659  },
      reference{-2.5,   1.0,   -2.3441906524655565, -8.304127986657925 },
      reference{-10.3,  0.5,   -15.549625455096617, -32.779606196567116},
      reference{20.0,   -30.0, 21.34507449386348,   -96.71434768953617 },
      reference{0.25,   -0.1,  1.207837423685703,   0.40303113537500845},
      reference{-0.5,   13.9,  -23.547665795621583, 21.079501339216186 },
      reference{-40.25, -3.0,  -118.72331803493407, 116.89475272889086 },
      reference{6.5,    0.75,  5.615914217052961,   1.3466143869872227 },
   };
   for (reference const& value : references) {
      complex_float8 const result =
         cat::lgamma(complex_float8(float8(value.real), float8(value.imag)));
      cat::verify(is_near(real_of(result), value.expected_real, 1.e-13));
      cat::verify(is_near(imag_of(result), value.expected_imag, 1.e-13));
   }

   // `lgamma(z + 1) = lgamma(z) + log(z)` across every region's boundary.
   cat::array<complex_float8, 7u> const boundaries{
      complex_float8{0.49 + 0.2i},   complex_float8{0.51 - 0.2i},
      complex_float8{6.99 + 1.0i},   complex_float8{-3.7 + 5.0i},
      complex_float8{2.0 + 13.99i},  complex_float8{-6.0 - 14.01i},
      complex_float8{-0.5 + 1.e-9i},
   };
   for (complex_float8 const& value : boundaries) {
      complex_float8 const step =
         cat::lgamma(value + 1.0) - cat::lgamma(value) - cat::log(value);
      cat::verify(is_near(real_of(step), 0.0, 1.e-12));
      cat::verify(is_near(imag_of(step), 0.0, 1.e-12));
   }

   // The real axis matches `cat::lgamma()`, with the branch cut's phase on
   // the negative side.
   complex_float8 const positive = cat::lgamma(complex_float8{3.5 + 0.0i});
   cat::verify(real_of(positive) == cat::make_raw_arithmetic(cat::lgamma(3.5)));
   cat::verify(imag_of(positive) == 0.0 && !has_sign(imag_of(positive)));
   complex_float8 const above_cut = cat::lgamma(complex_float8{-0.5 + 0.0i});
   cat::verify(
      real_of(above_cut) == cat::make_raw_arithmetic(cat::lgamma(-0.5))
   );
   cat::verify(imag_of(above_cut) == -math_pi);
   complex_float8 const below_cut =
      cat::lgamma(complex_float8(float8(-1.5), float8(-0.0)));
   cat::verify(imag_of(below_cut) == 2 * math_pi);
   complex_float8 const pole = cat::lgamma(complex_float8{-2.0 + 0.0i});
   cat::verify(
      real_of(pole) == cat::infinity && __builtin_isnan(imag_of(pole))
   );
   complex_float8 const not_a_number =
      cat::lgamma(complex_float8(float8(__builtin_nan("")), float8(1.0)));
   cat::verify(__builtin_isnan(real_of(not_a_number)));
   complex_float8 const upward =
      cat::lgamma(complex_float8(float8(1.0), float8(cat::infinity)));
   cat::verify(real_of(upward) == -cat::infinity);
   cat::verify(imag_of(upward) == cat::infinity);

   complex_float4 const single = cat::lgamma(complex_float4{0.f + 1.fi});
   cat::verify(is_near(real_of(single), -0.6509231993018432, 1.e-5));
   cat::verify(is_near(imag_of(single), -1.8724366472624272, 1.e-5));

   constexpr auto constant = cat::lgamma(complex_float8{0.0 + 1.0i});
   static_assert(is_near_constant(real_of(constant), -0.6509231993018432));
   static_assert(is_near_constant(imag_of(constant), -1.8724366472624272));
}

$test(complex_math_edges) {
   for (edge_case<double> const& parts : math_cases) {
      double const x = parts.real;
      double const y = parts.imag;
      complex_float8 const z = make_case(parts);
      bool const x_nan = __builtin_isnan(x);
      bool const y_nan = __builtin_isnan(y);
      bool const x_inf = __builtin_isinf(x);
      bool const y_inf = __builtin_isinf(y);
      bool const x_finite = __builtin_isfinite(x);
      bool const y_finite = __builtin_isfinite(y);

      // `abs`
      double const magnitude = cat::make_raw_arithmetic(cat::abs(z));
      switch (classify(x, y)) {
         case edge::zero:
            cat::verify(magnitude == 0.0 && !has_sign(magnitude));
            break;
         case edge::non_zero:
            cat::verify(__builtin_isfinite(magnitude) && magnitude > 0.0);
            break;
         case edge::infinite:
            cat::verify(__builtin_isinf(magnitude) && magnitude > 0.0);
            break;
         case edge::nan:
         case edge::non_zero_nan:
            cat::verify(__builtin_isnan(magnitude));
            break;
      }

      // `arg`
      double const phase = cat::make_raw_arithmetic(cat::arg(z));
      if (x_nan || y_nan) {
         cat::verify(__builtin_isnan(phase));
      } else if (classify(x, y) == edge::zero) {
         if (has_sign(x)) {
            cat::verify(is_about(phase, has_sign(y) ? -math_pi : math_pi));
         } else {
            cat::verify(has_sign(y) == has_sign(phase));
         }
      } else if (classify(x, y) == edge::non_zero) {
         if (x == 0.0) {
            cat::verify(is_about(phase, y < 0.0 ? -math_pi / 2 : math_pi / 2));
         } else if (y == 0.0) {
            if (x < 0.0) {
               cat::verify(is_about(phase, has_sign(y) ? -math_pi : math_pi));
            } else {
               cat::verify(phase == 0.0 && has_sign(y) == has_sign(phase));
            }
         } else {
            cat::verify(y > 0.0 ? phase > 0.0 : phase < 0.0);
         }
      } else if (x_inf && y_inf) {
         double const quarter = x < 0.0 ? 0.75 * math_pi : 0.25 * math_pi;
         cat::verify(is_about(phase, y > 0.0 ? quarter : -quarter));
      } else if (x_inf) {
         if (x < 0.0) {
            cat::verify(is_about(phase, has_sign(y) ? -math_pi : math_pi));
         } else {
            cat::verify(phase == 0.0 && has_sign(phase) == has_sign(y));
         }
      } else {
         cat::verify(is_about(phase, y < 0.0 ? -math_pi / 2 : math_pi / 2));
      }

      // `exp`
      complex_float8 const exponential = cat::exp(z);
      double const er = real_of(exponential);
      double const ei = imag_of(exponential);
      if (x == 0.0 && y == 0.0) {
         cat::verify(er == 1.0 && ei == 0.0 && has_sign(y) == has_sign(ei));
      } else if (x_finite && (y_inf || y_nan)) {
         cat::verify(__builtin_isnan(er) && __builtin_isnan(ei));
      } else if (x_inf && x > 0.0 && y == 0.0) {
         cat::verify(__builtin_isinf(er) && er > 0.0);
         cat::verify(ei == 0.0 && has_sign(y) == has_sign(ei));
      } else if (x_inf && x < 0.0 && (y_inf || y_nan)) {
         cat::verify(er == 0.0 && ei == 0.0);
      } else if (x_inf && x > 0.0 && (y_inf || y_nan)) {
         cat::verify(__builtin_isinf(er) && __builtin_isnan(ei));
      } else if (x_nan && y == 0.0) {
         cat::verify(__builtin_isnan(er));
         cat::verify(ei == 0.0 && has_sign(y) == has_sign(ei));
      } else if (x_nan) {
         cat::verify(__builtin_isnan(er) && __builtin_isnan(ei));
      } else if (y_finite && __builtin_fabs(y) <= 1.0) {
         cat::verify(!has_sign(er) && has_sign(ei) == has_sign(y));
      } else if (__builtin_isinf(er) && y == 0.0) {
         cat::verify(ei == 0.0 && has_sign(y) == has_sign(ei));
      }

      // `log`
      complex_float8 const logarithm = cat::log(z);
      double const lr = real_of(logarithm);
      double const li = imag_of(logarithm);
      double const absolute = cat::make_raw_arithmetic(cat::abs(z));
      if (x == 0.0 && y == 0.0) {
         cat::verify(__builtin_isinf(lr) && lr < 0.0);
         if (has_sign(x)) {
            cat::verify(is_about(li, has_sign(y) ? -math_pi : math_pi));
         } else {
            cat::verify(li == 0.0 && has_sign(y) == has_sign(li));
         }
      } else if (x_finite && y_inf) {
         cat::verify(__builtin_isinf(lr) && lr > 0.0);
         cat::verify(is_about(li, y > 0.0 ? math_pi / 2 : -math_pi / 2));
      } else if (x_finite && y_nan) {
         cat::verify(__builtin_isnan(lr) && __builtin_isnan(li));
      } else if (x_inf && x < 0.0 && y_finite) {
         cat::verify(__builtin_isinf(lr) && lr > 0.0);
         cat::verify(is_about(li, li > 0.0 ? math_pi : -math_pi));
      } else if (x_inf && x > 0.0 && y_finite) {
         cat::verify(__builtin_isinf(lr) && lr > 0.0);
         cat::verify(li == 0.0 && has_sign(y) == has_sign(li));
      } else if (x == 1.0 && y == 0.0) {
         cat::verify(lr == 0.0 && has_sign(li) == has_sign(y));
      } else if (x == 0.0 && y == 1.0) {
         cat::verify(lr == 0.0 && is_about(li, math_pi / 2));
      } else if (x == -1.0 && y == 0.0) {
         cat::verify(lr == 0.0);
         cat::verify(is_about(li, has_sign(y) ? -math_pi : math_pi));
      } else if (x == 0.0 && y == -1.0) {
         cat::verify(lr == 0.0 && is_about(li, -math_pi / 2));
      } else if (x_finite && y_finite && absolute < 1.0) {
         cat::verify(has_sign(lr) && has_sign(li) == has_sign(y));
      } else if (x_finite && y_finite && absolute > 1.0) {
         cat::verify(!has_sign(lr) && has_sign(li) == has_sign(y));
      }

      // `sqrt`
      complex_float8 const root = cat::sqrt(z);
      double const sr = real_of(root);
      double const si = imag_of(root);
      if (x == 0.0 && y == 0.0) {
         cat::verify(!has_sign(sr) && has_sign(si) == has_sign(y));
      } else if (y_inf) {
         cat::verify(__builtin_isinf(sr) && sr > 0.0);
         cat::verify(__builtin_isinf(si) && has_sign(si) == has_sign(y));
      } else if (x_finite && y_nan) {
         cat::verify(__builtin_isnan(sr) && __builtin_isnan(si));
      } else if (x_inf && x < 0.0 && y_finite) {
         cat::verify(sr == 0.0 && !has_sign(sr));
         cat::verify(__builtin_isinf(si) && has_sign(y) == has_sign(si));
      } else if (x_inf && x > 0.0 && y_finite) {
         cat::verify(__builtin_isinf(sr) && sr > 0.0);
         cat::verify(si == 0.0 && has_sign(y) == has_sign(si));
      } else if (x_inf && x < 0.0 && y_nan) {
         cat::verify(__builtin_isnan(sr) && __builtin_isinf(si));
      } else if (x_inf && x > 0.0 && y_nan) {
         cat::verify(__builtin_isinf(sr) && sr > 0.0 && __builtin_isnan(si));
      } else if (x_nan) {
         cat::verify(__builtin_isnan(sr) && __builtin_isnan(si));
      } else {
         cat::verify(!has_sign(sr) && has_sign(si) == has_sign(y));
      }

      // `sinh`
      complex_float8 const sine = cat::sinh(z);
      double const hr = real_of(sine);
      double const hi = imag_of(sine);
      if (x == 0.0 && y == 0.0) {
         cat::verify(hr == 0.0 && has_sign(hr) == has_sign(x));
         cat::verify(hi == 0.0 && has_sign(hi) == has_sign(y));
      } else if (x == 0.0 && (y_inf || y_nan)) {
         cat::verify(hr == 0.0 && __builtin_isnan(hi));
      } else if (x_finite && (y_inf || y_nan)) {
         cat::verify(__builtin_isnan(hr) && __builtin_isnan(hi));
      } else if (x_inf && y == 0.0) {
         cat::verify(__builtin_isinf(hr) && has_sign(hr) == has_sign(x));
         cat::verify(hi == 0.0 && has_sign(hi) == has_sign(y));
      } else if (x_inf && y_finite) {
         cat::verify(__builtin_isinf(hr) && __builtin_isinf(hi));
         cat::verify(
            has_sign(hr) == has_sign(x * cat::detail::raw_cos(y))
            && has_sign(hi) == has_sign(cat::detail::raw_sin(y))
         );
      } else if (x_inf) {
         cat::verify(__builtin_isinf(hr) && __builtin_isnan(hi));
      } else if (x_nan && y == 0.0) {
         cat::verify(__builtin_isnan(hr));
         cat::verify(hi == 0.0 && has_sign(hi) == has_sign(y));
      } else if (x_nan) {
         cat::verify(__builtin_isnan(hr) && __builtin_isnan(hi));
      }

      // `cosh`
      complex_float8 const cosine = cat::cosh(z);
      double const cr = real_of(cosine);
      double const ci = imag_of(cosine);
      if (x == 0.0 && y == 0.0) {
         cat::verify(cr == 1.0 && ci == 0.0 && has_sign(ci) == has_sign(y));
      } else if (x == 0.0 && (y_inf || y_nan)) {
         cat::verify(__builtin_isnan(cr) && ci == 0.0);
      } else if (x_finite && (y_inf || y_nan)) {
         cat::verify(__builtin_isnan(cr) && __builtin_isnan(ci));
      } else if (x_inf && y == 0.0) {
         cat::verify(__builtin_isinf(cr) && !has_sign(cr));
         cat::verify(ci == 0.0 && has_sign(ci) == has_sign(y));
      } else if (x_inf && y_finite) {
         cat::verify(__builtin_isinf(cr) && __builtin_isinf(ci));
         cat::verify(
            has_sign(cr) == has_sign(cat::detail::raw_cos(y))
            && has_sign(ci) == has_sign(x * cat::detail::raw_sin(y))
         );
      } else if (x_inf && y_inf) {
         cat::verify(__builtin_isinf(cr) && __builtin_isnan(ci));
      } else if (x_inf) {
         cat::verify(__builtin_isinf(cr) && cr > 0.0 && __builtin_isnan(ci));
      } else if (x_nan && y == 0.0) {
         cat::verify(__builtin_isnan(cr) && ci == 0.0);
      } else if (x_nan) {
         cat::verify(__builtin_isnan(cr) && __builtin_isnan(ci));
      }

      // `tanh`
      complex_float8 const tangent = cat::tanh(z);
      double const tr = real_of(tangent);
      double const ti = imag_of(tangent);
      if (x == 0.0 && y == 0.0) {
         cat::verify(tr == 0.0 && has_sign(tr) == has_sign(x));
         cat::verify(ti == 0.0 && has_sign(ti) == has_sign(y));
      } else if (x_finite && (y_inf || y_nan)) {
         cat::verify(__builtin_isnan(tr) && __builtin_isnan(ti));
      } else if (x_inf && y_finite) {
         cat::verify(tr == (x > 0.0 ? 1.0 : -1.0) && ti == 0.0);
         cat::verify(has_sign(ti) == has_sign(cat::detail::raw_sin(2 * y)));
      } else if (x_inf) {
         cat::verify(tr == (x > 0.0 ? 1.0 : -1.0) && ti == 0.0);
      } else if (x_nan && y == 0.0) {
         cat::verify(__builtin_isnan(tr));
         cat::verify(ti == 0.0 && has_sign(ti) == has_sign(y));
      } else if (x_nan) {
         cat::verify(__builtin_isnan(tr) && __builtin_isnan(ti));
      }

      // `asin`
      auto const arcsine = cat::asin(z);
      double const ar = real_of(arcsine);
      double const ai = imag_of(arcsine);
      if (x == 0.0 && y == 0.0) {
         cat::verify(
            has_sign(ar) == has_sign(x) && has_sign(ai) == has_sign(y)
         );
      } else if (x_finite && y_inf) {
         cat::verify(ar == 0.0 && has_sign(x) == has_sign(ar));
         cat::verify(__builtin_isinf(ai) && has_sign(y) == has_sign(ai));
      } else if (x == 0.0 && y_nan) {
         cat::verify(ar == 0.0 && has_sign(x) == has_sign(ar));
         cat::verify(__builtin_isnan(ai));
      } else if (x_finite && y_nan) {
         cat::verify(__builtin_isnan(ar) && __builtin_isnan(ai));
      } else if (x_inf && y_finite) {
         cat::verify(is_about(ar, x > 0.0 ? math_pi / 2 : -math_pi / 2));
         cat::verify(__builtin_isinf(ai) && has_sign(y) == has_sign(ai));
      } else if (x_inf && y_inf) {
         cat::verify(is_about(ar, has_sign(x) ? -math_pi / 4 : math_pi / 4));
         cat::verify(__builtin_isinf(ai) && has_sign(y) == has_sign(ai));
      } else if (x_inf) {
         cat::verify(__builtin_isnan(ar) && __builtin_isinf(ai));
         cat::verify(has_sign(x) != has_sign(ai));
      } else if (x_nan && y_inf) {
         cat::verify(__builtin_isnan(ar) && __builtin_isinf(ai));
      } else if (x_nan) {
         cat::verify(__builtin_isnan(ar) && __builtin_isnan(ai));
      } else {
         cat::verify(
            has_sign(ar) == has_sign(x) && has_sign(ai) == has_sign(y)
         );
      }

      // `acos`
      auto const arccosine = cat::acos(z);
      double const or_ = real_of(arccosine);
      double const oi = imag_of(arccosine);
      if (x == 0.0 && y == 0.0) {
         cat::verify(is_about(or_, math_pi / 2));
         cat::verify(oi == 0.0 && has_sign(y) != has_sign(oi));
      } else if (x == 0.0 && y_nan) {
         cat::verify(is_about(or_, math_pi / 2) && __builtin_isnan(oi));
      } else if (x_finite && y_inf) {
         cat::verify(is_about(or_, math_pi / 2));
         cat::verify(__builtin_isinf(oi) && has_sign(y) != has_sign(oi));
      } else if (x_finite && y_nan) {
         cat::verify(__builtin_isnan(or_) && __builtin_isnan(oi));
      } else if (x_inf && y_finite) {
         if (x < 0.0) {
            cat::verify(is_about(or_, math_pi));
         } else {
            cat::verify(or_ == 0.0 && !has_sign(or_));
         }

         cat::verify(__builtin_isinf(oi) && has_sign(y) != has_sign(oi));
      } else if (x_inf && y_inf) {
         cat::verify(is_about(or_, (x < 0.0 ? 0.75 : 0.25) * math_pi));
         cat::verify(__builtin_isinf(oi) && has_sign(y) != has_sign(oi));
      } else if (x_inf) {
         cat::verify(__builtin_isnan(or_) && __builtin_isinf(oi));
      } else if (x_nan && y_inf) {
         cat::verify(__builtin_isnan(or_) && __builtin_isinf(oi));
         cat::verify(has_sign(y) != has_sign(oi));
      } else if (x_nan) {
         cat::verify(__builtin_isnan(or_) && __builtin_isnan(oi));
      } else {
         cat::verify(!has_sign(or_) && has_sign(oi) != has_sign(y));
      }

      // `atanh`, which `atan` rotates.
      auto const area_tangent =
         cat::detail::complex_atanh<double>(cat::detail::complex_load(z));
      double const nr = __real area_tangent;
      double const ni = __imag area_tangent;
      if (x == 0.0 && y == 0.0) {
         cat::verify(
            has_sign(nr) == has_sign(x) && has_sign(ni) == has_sign(y)
         );
      } else if (x == 0.0 && y_nan) {
         cat::verify(nr == 0.0 && has_sign(x) == has_sign(nr));
         cat::verify(__builtin_isnan(ni));
      } else if (__builtin_fabs(x) == 1.0 && y == 0.0) {
         cat::verify(__builtin_isinf(nr) && has_sign(x) == has_sign(nr));
         cat::verify(ni == 0.0 && has_sign(y) == has_sign(ni));
      } else if ((x_finite || x_inf || x_nan) && y_inf) {
         cat::verify(nr == 0.0 && has_sign(x) == has_sign(nr));
         cat::verify(is_about(ni, has_sign(y) ? -math_pi / 2 : math_pi / 2));
      } else if (x_finite && y_nan) {
         cat::verify(__builtin_isnan(nr) && __builtin_isnan(ni));
      } else if (x_inf && y_finite) {
         cat::verify(nr == 0.0 && has_sign(x) == has_sign(nr));
         cat::verify(is_about(ni, has_sign(y) ? -math_pi / 2 : math_pi / 2));
      } else if (x_inf && y_nan) {
         cat::verify(nr == 0.0 && has_sign(x) == has_sign(nr));
         cat::verify(__builtin_isnan(ni));
      } else if (x_nan) {
         cat::verify(__builtin_isnan(nr) && __builtin_isnan(ni));
      } else {
         cat::verify(
            has_sign(nr) == has_sign(x) && has_sign(ni) == has_sign(y)
         );
      }

      // `sin`, `cos`, `tan`, and `atan` are rotations of the hyperbolic
      // functions.
      complex_float8 const rotated = make_case({-y, x});
      auto const angle = make_angle_case(parts);
      complex_float8 const hyperbolic_sine = cat::sinh(rotated);
      complex_float8 const circular_sine = cat::sin(angle);
      cat::verify(
         is_identical(real_of(circular_sine), imag_of(hyperbolic_sine))
      );
      cat::verify(
         is_identical(imag_of(circular_sine), -real_of(hyperbolic_sine))
      );
      complex_float8 const hyperbolic_cosine = cat::cosh(rotated);
      complex_float8 const circular_cosine = cat::cos(angle);
      cat::verify(
         is_identical(real_of(circular_cosine), real_of(hyperbolic_cosine))
      );
      cat::verify(
         is_identical(imag_of(circular_cosine), imag_of(hyperbolic_cosine))
      );
      complex_float8 const hyperbolic_tangent = cat::tanh(rotated);
      complex_float8 const circular_tangent = cat::tan(angle);
      cat::verify(
         is_identical(real_of(circular_tangent), imag_of(hyperbolic_tangent))
      );
      cat::verify(
         is_identical(imag_of(circular_tangent), -real_of(hyperbolic_tangent))
      );
      auto const rotated_area_tangent =
         cat::detail::complex_atanh<double>(cat::detail::complex_load(rotated));
      auto const arctangent = cat::atan(z);
      cat::verify(
         is_identical(real_of(arctangent), __imag rotated_area_tangent)
      );
      cat::verify(
         is_identical(imag_of(arctangent), -__real rotated_area_tangent)
      );
   }
}

$test(complex_math_pow_edges) {
   // Every power is `exp(exponent * log(base))`.
   for (edge_case<double> const& base_parts : math_cases) {
      complex_float8 const base = make_case(base_parts);
      complex_float8 const logarithm = cat::log(base);
      complex_float8 const real_logarithm =
         cat::log(complex_float8(float8(base_parts.real)));
      for (edge_case<double> const& exponent_parts : math_cases) {
         complex_float8 const exponent = make_case(exponent_parts);
         complex_float8 const power = cat::pow(base, exponent);
         complex_float8 const expected = cat::exp(exponent * logarithm);
         cat::verify(is_identical(real_of(power), real_of(expected)));
         cat::verify(is_identical(imag_of(power), imag_of(expected)));

         complex_float8 const real_power = cat::pow(base, exponent_parts.real);
         complex_float8 const real_expected =
            cat::exp(complex_float8(float8(exponent_parts.real)) * logarithm);
         cat::verify(is_identical(real_of(real_power), real_of(real_expected)));
         cat::verify(is_identical(imag_of(real_power), imag_of(real_expected)));

         complex_float8 const scalar_power =
            cat::pow(base_parts.real, exponent);
         complex_float8 const scalar_expected =
            cat::exp(exponent * real_logarithm);
         cat::verify(
            is_identical(real_of(scalar_power), real_of(scalar_expected))
         );
         cat::verify(
            is_identical(imag_of(scalar_power), imag_of(scalar_expected))
         );
      }
   }
}

// The policy accessors of an l-value return proxy references to it, as they do
// for `basic_int` and `basic_float`. An r-value returns a copy instead.
$test(complex_policy_references) {
   using cat::int1;
   complex_int1 value(int1(100), int1(-100));
   complex_int1 const constant = value;
   static_assert(cat::is_same<
                 decltype(value.sat()),
                 cat::complex_overflow_reference<
                    complex_int1, cat::overflow_policies::saturate>>);
   static_assert(cat::is_same<
                 decltype(constant.wrap()),
                 cat::complex_overflow_reference<
                    complex_int1 const, cat::overflow_policies::wrap>>);
   static_assert(
      cat::is_same<decltype(complex_int1{}.sat()), sat_complex_int1>
   );
   static_assert(
      cat::is_same<decltype(cat::move(value.wrap()).sat()), sat_complex_int1>
   );

   // Arithmetic reads the wrapped value with the reference's policy, and the
   // left operand's policy is kept.
   static_assert(cat::is_same<decltype(value.sat() + value), sat_complex_int1>);
   static_assert(cat::is_same<decltype(value + value.sat()), complex_int1>);
   static_assert(
      cat::is_same<decltype(value.sat() * value.wrap()), sat_complex_int1>
   );
   cat::verify(value.sat() + value == sat_complex_int1(int1(127), int1(-128)));
   cat::verify(value.wrap() + value == wrap_complex_int1(int1(-56), int1(56)));
   cat::verify(-value.sat() == sat_complex_int1(int1(-100), int1(100)));
   cat::verify(value.sat() == value && value == value.wrap());
   cat::verify(value.sat().real() == cat::sat_int1(100));

   // Compound assignment writes through the reference.
   value.sat() += value;
   cat::verify(value == complex_int1(int1(127), int1(-128)));
   value.wrap() -= complex_int1(int1(1), int1(1));
   cat::verify(value == complex_int1(int1(126), int1(127)));
   value.sat() *= 2;
   cat::verify(value == complex_int1(int1(127), int1(127)));
   // A reference's own accessors rebind it to another policy.
   auto saturating = value.wrap();
   saturating.sat() /= complex_int1(int1(0), int1(1));
   cat::verify(value == complex_int1(int1(127), int1(-127)));
   static_assert(
      !cat::is_add_assignable<decltype(constant.sat()), complex_int1>
   );

   complex_float8 number{1.0 + 2.0i};
   static_assert(cat::is_same<
                 decltype(number.fast()),
                 cat::complex_precision_reference<
                    complex_float8, cat::precision_policies::fast>>);
   static_assert(
      cat::is_same<decltype(number.fast() * 2.0), complex_float8_fast>
   );
   static_assert(
      cat::is_same<decltype(number.fast().precise() + 1.0), complex_float8>
   );
   number.fast() *= 2.0;
   cat::verify(number == complex_float8{2.0 + 4.0i});
   number.fast() += number.precise();
   cat::verify(number == complex_float8{4.0 + 8.0i});
}

// Assigning a reference of the same type writes through it. A reference to
// `const` cannot be assigned, and an r-value, `const` or not, gives a copy.
$test(complex_reference_assignment) {
   using cat::int1;
   complex_int1 first(int1(1), int1(2));
   complex_int1 second(int1(3), int1(4));
   auto first_ref = first.sat();
   first_ref = second.sat();
   cat::verify(first == second);
   first.wrap() = complex_int1(int1(5), int1(6)).wrap();
   cat::verify(first == complex_int1(int1(5), int1(6)));
   complex_float8 number{1.0 + 2.0i};
   complex_float8 const other_number{3.0 + 4.0i};
   number.fast() = other_number.fast();
   cat::verify(number == other_number);

   complex_int1 const constant = first;
   static_assert(
      cat::is_assignable<decltype(first.sat())&, decltype(first.sat())>
   );
   static_assert(
      !cat::is_assignable<decltype(constant.sat())&, decltype(constant.sat())>
   );
   static_assert(
      !cat::is_assignable<
         decltype(other_number.fast())&, decltype(other_number.fast())>
   );

   static_assert(
      cat::is_same<decltype(cat::move(constant).sat()), sat_complex_int1>
   );
   static_assert(
      cat::is_same<
         decltype(cat::move(other_number).fast()), complex_float8_fast>
   );
   // An r-value copy keeps the parts bit for bit.
   constexpr complex_int1 most_negative(int1(-128), int1(127));
   static_assert(
      complex_int1(most_negative).sat()
      == sat_complex_int1(cat::sat_int1(-128), cat::sat_int1(127))
   );
}

// A reference converts implicitly into its own value with its policy and
// explicitly into anything that value converts into. The `<cat/math>`
// functions, `cat::conj()`, structured bindings, and `cat::fmt()` accept it as
// a complex number with its policy.
$test(complex_reference_as_value) {
   using cat::int1;
   complex_int1 value(int1(1), int1(-2));
   complex_float8 number{0.0 + 0.0i};
   sat_complex_int1 const saturated = value.sat();
   complex_float8_fast const fast = number.fast();
   cat::verify(saturated == value && fast == number);
   static_assert(!cat::is_convertible<decltype(value.sat()), complex_int1>);
   static_assert(cat::is_constructible<complex_int1, decltype(value.sat())>);
   static_assert(cat::is_complex<decltype(number.fast())>);

   static_assert(
      cat::is_same<decltype(cat::conj(value.sat())), sat_complex_int1>
   );
   cat::verify(
      cat::conj(value.sat())
      == sat_complex_int1(cat::sat_int1(1), cat::sat_int1(2))
   );
   static_assert(
      cat::is_same<decltype(cat::exp(number.fast())), complex_float8_fast>
   );
   cat::verify(cat::exp(number.fast()) == complex_float8{1.0 + 0.0i});
   cat::verify(cat::abs(number.fast()) == 0.0);

   auto [real, imag] = value.sat();
   static_assert(cat::is_same<decltype(real), cat::sat_int1>);
   cat::verify(real == 1 && imag == -2);

   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   auto allocator = cat::make_allocator_linear(page);
   complex_float8 printed{1.5 + 2.5i};
   cat::verify(
      cat::fmt(allocator, "{} {}", value.sat(), printed.fast()).verify()
      == "1 - 2i 1.5E0 + 2.5E0i"
   );
}
