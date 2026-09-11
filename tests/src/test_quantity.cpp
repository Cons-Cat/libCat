#include <cat/format>
#include <cat/linear_allocator>
#include <cat/math>
#include <cat/page_allocator>
#include <cat/quantity>
#include <cat/simd>

#include "../unit_tests.hpp"

using namespace cat::literals;

namespace {

template <auto quantity>
using quantity_int = cat::basic_int<
   cat::int8::raw_type, cat::overflow_policies::undefined, quantity>;

template <auto quantity>
using quantity_uint = cat::basic_int<
   cat::uint8::raw_type, cat::overflow_policies::undefined, quantity>;

template <auto quantity>
using quantity_float = cat::basic_float<
   cat::float8::raw_type, cat::precision_policies::precise, quantity>;

using meter_int = quantity_int<cat::units::metre>;
using meter_uint = quantity_uint<cat::units::metre>;
using second_int = quantity_int<cat::units::second>;
using gram_int = quantity_int<cat::units::gram>;
using kilogram_int = quantity_int<cat::units::kilogram>;
using meter_float = quantity_float<cat::units::metre>;
using second_float = quantity_float<cat::units::second>;
inline constexpr auto kilometre = cat::units::kilo<cat::units::metre>;
using kilometre_float = quantity_float<kilometre>;

inline constexpr auto metre_second = cat::units::metre * cat::units::second;
inline constexpr auto square_metre = cat::pow<2>(cat::units::metre);
inline constexpr auto cubic_metre = cat::pow<3>(cat::units::metre);
using metre_second_int = quantity_int<metre_second>;
using metre_second_float = quantity_float<metre_second>;
using square_metre_float = quantity_float<square_metre>;
using cubic_metre_float = quantity_float<cubic_metre>;

inline constexpr struct ratio : cat::quantity_spec<cat::dimension<>{}> {
} ratio;

using ratio_quantity =
   cat::reference<__typeof_unqual(ratio), cat::unit<>>;
inline constexpr ratio_quantity ratio_reference;
using ratio_float = quantity_float<ratio_reference>;

struct item_dimension : cat::base_dimension<"item"> {};

using item_dimension_type =
   cat::dimension<cat::dimension_power<item_dimension, 1>>;

struct item_count : cat::quantity_spec<item_dimension_type{}> {};

struct item : cat::named_unit<"item", item_count{}> {};

using items =
   cat::reference<item_count, cat::unit<cat::unit_power<item, 1>>>;

struct damaged_item_count : cat::quantity_spec<item_dimension_type{}> {};

using damaged_items = cat::reference<
   damaged_item_count, cat::unit<cat::unit_power<item, 1>>>;
inline constexpr items item_reference;
inline constexpr damaged_items damaged_item_reference;
using item_int = quantity_int<item_reference>;
using damaged_item_int = quantity_int<damaged_item_reference>;

inline constexpr struct width : cat::quantity_spec<cat::units::length> {
} width;

inline constexpr struct height : cat::quantity_spec<cat::units::length> {
} height;

inline constexpr struct area
    : cat::quantity_spec<cat::pow<2>(cat::units::length)> {
} area;

inline constexpr struct horizontal_area
    : cat::quantity_spec<area, width * height> {
} horizontal_area;

using horizontal_area_spec = __typeof_unqual(horizontal_area);

inline constexpr auto width_in_metres = width[cat::units::metre];
inline constexpr auto height_in_metres = height[cat::units::metre];
using width_int = quantity_int<width_in_metres>;
using height_int = quantity_int<height_in_metres>;

inline constexpr struct displacement
    : cat::quantity_spec<
         cat::units::dim_length, cat::quantity_character{
                                    cat::quantity_tensor_order::vector,
                                    cat::quantity_field::real,
                                 }> {
} displacement;

using displacement_spec = __typeof_unqual(displacement);

inline constexpr struct double_metre
    : cat::prefixed_unit<"2m", cat::mag_ratio<2>, cat::units::metre> {
} double_metre;

using double_metre_int = quantity_int<double_metre>;

inline constexpr struct minute
    : cat::named_unit<"min", cat::mag<60> * cat::units::second> {
} minute;

using minute_int = quantity_int<minute>;

struct duplicate_item_dimension : cat::base_dimension<"item"> {};

using duplicate_item_dimension_type =
   cat::dimension<cat::dimension_power<duplicate_item_dimension, 1>>;

template <typename From, typename To>
concept can_cast = requires(From from) { static_cast<To>(from); };

template <typename Vector, typename Lane>
concept can_load =
   requires(Vector vector, Lane const* p_lanes) { vector.load(p_lanes); };

template <typename Vector>
concept can_reduce_mul = requires(Vector vector) { vector.reduce_mul(); };

template <typename T>
concept has_transcendental_math = requires(T value) {
                                     cat::exp(value);
                                     cat::log(value);
                                     cat::asin(value);
                                     cat::acos(value);
                                     cat::atan(value);
                                     cat::sinh(value);
                                     cat::cosh(value);
                                     cat::tanh(value);
                                     cat::lgamma(value);
                                  };

template <typename T>
concept has_circular_math = requires(T value) {
                               cat::sin(value);
                               cat::cos(value);
                               cat::tan(value);
                            };

template <typename T>
concept has_runtime_power = requires(T value) { cat::pow(value, 2); };

template <typename T>
concept has_runtime_nroot = requires(T value) { cat::nroot(value, 2); };

template <typename T, typename U, typename V>
concept can_fma = requires(T value, U multiplier, V addend) {
                     cat::fma(value, multiplier, addend);
                  };

}  // namespace

$test(quantity_math_preserving) {
   constexpr meter_int negative_metres(-5);
   static_assert(cat::abs(negative_metres) == meter_int(5));
   static_assert(cat::is_same<decltype(cat::abs(negative_metres)), meter_int>);

   constexpr meter_float fractional_metres(2.75);
   static_assert(cat::floor(fractional_metres).raw == 2.0);
   static_assert(cat::ceil(fractional_metres).raw == 3.0);
   static_assert(
      cat::is_same<decltype(cat::floor(fractional_metres)), meter_float>
   );

   static_assert(cat::min(meter_int(3), meter_int(2)) == meter_int(2));
   static_assert(cat::max(meter_int(3), meter_int(2)) == meter_int(3));
   static_assert(
      cat::clamp(meter_int(12), meter_int(0), meter_int(10)) == meter_int(10)
   );
   static_assert(
      cat::sum(meter_int(1), meter_int(2), meter_int(3)) == meter_int(6)
   );

   static_assert(cat::is_less(meter_int(1), meter_int(2)));
   static_assert(cat::is_greater(meter_int(2), meter_int(1)));
   static_assert(cat::is_less_equal(meter_int(2), meter_int(2)));
   static_assert(cat::is_greater_equal(meter_int(2), meter_int(2)));
   static_assert(cat::is_equal_to(meter_int(2), meter_int(2)));
   static_assert(cat::is_not_equal_to(meter_int(1), meter_int(2)));
   static_assert(cat::compare_three_way(meter_int(1), meter_int(2)) < 0);
   static_assert(cat::is_less(meter_int(1))(meter_int(2)));
}

$test(quantity_math_dimension_algebra) {
   constexpr cat::units::metre_wrap_int4 distance(12);
   constexpr cat::units::second_int4 duration(3);
   constexpr auto speed = distance / duration;
   static_assert(speed.raw == 4);
   static_assert(
      cat::make_overflow_policy<__typeof_unqual(speed)>
      == cat::overflow_policies::wrap
   );
   static_assert(cat::is_same<
                 decltype(speed)::quantity_type,
                 cat::units::metre_per_second_wrap_int4::quantity_type>);

   constexpr auto product = cat::product(meter_int(2), second_int(3));
   static_assert(product.raw == 6);
   static_assert(cat::is_same<
                 decltype(product)::quantity_type,
                 cat::reference_quantity<metre_second>>);

   constexpr auto dot =
      cat::dot(meter_int(2), second_int(3), meter_int(4), second_int(5));
   static_assert(dot.raw == 26);
   static_assert(
      cat::is_same<
         decltype(dot)::quantity_type, cat::reference_quantity<metre_second>>
   );

   constexpr square_metre_float area_value(9.0);
   constexpr auto side = cat::sqrt(area_value);
   static_assert(side.raw == 3.0);
   using side_quantity = decltype(side)::quantity_type;
   static_assert(
      cat::is_same<side_quantity::unit_type, cat::units::metres::unit_type>
   );
   static_assert(
      cat::is_same<
         side_quantity::dimension_type, cat::units::metres::dimension_type>
   );

   constexpr auto reciprocal_side = cat::rsqrt(area_value);
   static_assert(reciprocal_side.raw == 1.0 / 3.0);
   using reciprocal_side_quantity = decltype(reciprocal_side)::quantity_type;
   using inverse_metre_quantity =
      cat::reference_quantity<cat::inverse(cat::units::metre)>;
   static_assert(
      cat::is_same<
         reciprocal_side_quantity::unit_type, inverse_metre_quantity::unit_type>
   );

   constexpr cubic_metre_float volume_value(27.0);
   constexpr auto edge = cat::cbrt(volume_value);
   static_assert(edge.raw == 3.0);
   using edge_quantity = decltype(edge)::quantity_type;
   static_assert(
      cat::is_same<edge_quantity::unit_type, cat::units::metres::unit_type>
   );

   constexpr auto reciprocal_edge = cat::rcbrt(volume_value);
   static_assert(reciprocal_edge.raw == 1.0 / 3.0);
   using reciprocal_edge_quantity = decltype(reciprocal_edge)::quantity_type;
   static_assert(
      cat::is_same<
         reciprocal_edge_quantity::unit_type, inverse_metre_quantity::unit_type>
   );

   constexpr auto squared = cat::pow<2>(meter_float(4.0));
   static_assert(cat::abs(squared.raw - 16.0) < 1e-12);
   static_assert(cat::is_same<
                 decltype(squared)::quantity_type,
                 cat::reference_quantity<square_metre>>);

   constexpr auto rooted = cat::nroot<2>(area_value);
   static_assert(rooted.raw == 3.0);
   using rooted_quantity = decltype(rooted)::quantity_type;
   static_assert(
      cat::is_same<rooted_quantity::unit_type, cat::units::metres::unit_type>
   );

   static_assert(!has_runtime_power<meter_float>);
   static_assert(!has_runtime_nroot<meter_float>);
}

$test(quantity_math_fma_and_division) {
   constexpr auto integer_fma =
      cat::fma(meter_int(2), second_int(3), metre_second_int(4));
   static_assert(integer_fma.raw == 10);
   static_assert(cat::is_same<
                 decltype(integer_fma)::quantity_type,
                 cat::reference_quantity<metre_second>>);

   constexpr auto float_fma =
      cat::fma(meter_float(2.0), second_float(3.0), metre_second_float(4.0));
   static_assert(float_fma.raw == 10.0);
   static_assert(cat::is_same<
                 decltype(float_fma)::quantity_type,
                 cat::reference_quantity<metre_second>>);
   static_assert(!can_fma<meter_float, second_float, meter_float>);

   constexpr auto fast_fma =
      cat::fma(meter_float(2.0).fast(), 3.0, meter_float(4.0).fast());
   static_assert(fast_fma.raw == 10.0);
   static_assert(
      decltype(fast_fma)::precision_policy == cat::precision_policies::fast
   );
   static_assert(
      cat::is_same<decltype(fast_fma)::quantity_type, cat::units::metres>
   );

   constexpr auto converted_fma =
      cat::fma(meter_float(2.0), 3.0, kilometre_float(1.0));
   static_assert(converted_fma.raw == 1'006.0);

   constexpr auto integer_floor = cat::div_floor(meter_int(5), second_int(2));
   constexpr auto integer_ceil = cat::div_ceil(meter_int(5), second_int(2));
   static_assert(integer_floor.raw == 2);
   static_assert(integer_ceil.raw == 3);
   static_assert(
      cat::is_same<
         decltype(integer_floor)::quantity_type, cat::units::metres_per_second>
   );

   constexpr auto float_floor =
      cat::div_floor(meter_float(5.0), second_float(2.0));
   constexpr auto float_ceil =
      cat::div_ceil(meter_float(5.0), second_float(2.0));
   static_assert(float_floor.raw == 2.0);
   static_assert(float_ceil.raw == 3.0);
   static_assert(
      cat::is_same<
         decltype(float_floor)::quantity_type, cat::units::metres_per_second>
   );
}

$test(quantity_math_dimensionless_and_predicates) {
   constexpr ratio_float zero(0.0);
   constexpr ratio_float one(1.0);
   constexpr auto runtime_power = cat::pow(ratio_float(2.0), 2);
   constexpr auto runtime_root = cat::nroot(ratio_float(9.0), 2);
   constexpr auto runtime_reciprocal_root = cat::rnroot(ratio_float(4.0), 2);
   static_assert(cat::abs(runtime_power.raw - 4.0) < 1e-12);
   static_assert(cat::abs(runtime_root.raw - 3.0) < 1e-12);
   static_assert(cat::abs(runtime_reciprocal_root.raw - 0.5) < 1e-12);
   static_assert(has_transcendental_math<ratio_float>);
   static_assert(!has_transcendental_math<meter_float>);
   static_assert(cat::exp(zero).raw == 1.0);
   static_assert(
      cat::is_same<decltype(cat::exp(zero))::quantity_type, ratio_quantity>
   );
   static_assert(cat::log(one).raw == 0.0);
   static_assert(!has_circular_math<ratio_float>);
   static_assert(!has_circular_math<meter_float>);
   static_assert(has_circular_math<cat::units::degree_float8>);
   static_assert(has_circular_math<cat::units::radian_float8>);

   constexpr cat::units::degree_float8 thirty_degrees(30.0);
   constexpr cat::units::radian_float8 pi_over_six(cat::pi<double> / 6);
   constexpr auto degree_sine = cat::sin(thirty_degrees);
   constexpr auto radian_sine = cat::sin(pi_over_six);
   static_assert(cat::abs(degree_sine - radian_sine) < 1e-12);
   static_assert(cat::abs(degree_sine - 0.5) < 1e-12);
   static_assert(
      cat::is_same<decltype(degree_sine)::quantity_type, cat::scalar_quantity>
   );

   constexpr cat::units::degree_float8 sixty_degrees(60.0);
   constexpr cat::units::radian_float8 pi_over_three(cat::pi<double> / 3);
   static_assert(
      cat::abs(cat::cos(sixty_degrees) - cat::cos(pi_over_three)) < 1e-12
   );

   constexpr cat::units::degree_float8 forty_five_degrees(45.0);
   constexpr cat::units::radian_float8 pi_over_four(cat::pi<double> / 4);
   static_assert(
      cat::abs(cat::tan(forty_five_degrees) - cat::tan(pi_over_four)) < 1e-12
   );

   constexpr auto inverse_sine = cat::asin(zero);
   constexpr auto inverse_cosine = cat::acos(one);
   constexpr auto inverse_tangent = cat::atan(zero);
   static_assert(inverse_sine.raw == 0.0);
   static_assert(inverse_cosine.raw == 0.0);
   static_assert(inverse_tangent.raw == 0.0);
   static_assert(
      cat::is_same<decltype(inverse_sine)::quantity_type, cat::units::radians>
   );
   static_assert(
      cat::is_same<decltype(inverse_cosine)::quantity_type, cat::units::radians>
   );
   static_assert(
      cat::is_same<
         decltype(inverse_tangent)::quantity_type, cat::units::radians>
   );
   static_assert(cat::sinh(zero).raw == 0.0);
   static_assert(cat::cosh(zero).raw == 1.0);
   static_assert(cat::tanh(zero).raw == 0.0);
   static_assert(cat::abs(cat::lgamma(one).raw) < 1e-12);

   constexpr auto angle =
      cat::atan2(meter_float(1'000.0), kilometre_float(1.0));
   static_assert(
      cat::is_same<decltype(angle)::quantity_type, cat::units::radians>
   );
   static_assert(cat::abs(angle.raw - (cat::pi<double> / 4.0)) < 1e-12);

   static_assert(cat::is_even(meter_int(4)));
   static_assert(cat::is_odd(meter_int(3)));
   static_assert(cat::is_divisible_by(meter_int(12), meter_int(3)));
   static_assert(
      cat::round_up_to_multiple_of(meter_uint(5), meter_uint(2))
      == meter_uint(6)
   );
   static_assert(
      cat::round_down_to_multiple_of(meter_int(5), meter_int(2)) == meter_int(4)
   );
   static_assert(
      cat::round_to_multiple_of(meter_int(5), meter_int(2)) == meter_int(6)
   );

   constexpr meter_float infinite_metres = cat::infinity;
   static_assert(!cat::is_finite(infinite_metres));
   static_assert(
      cat::is_same<decltype(cat::pi<meter_float>), meter_float const>
   );
   static_assert(
      cat::is_same<decltype(cat::tau<meter_float>), meter_float const>
   );
   static_assert(cat::pi<meter_float>.raw > 3.0);
   static_assert(cat::tau<meter_float>.raw > 6.0);
}

$test(quantity) {
   static_assert(!cat::detail::has_binary_plus<meter_int, second_int>);
   static_assert(!cat::detail::has_binary_modulo_by<meter_int, meter_int>);
   static_assert(!cat::detail::has_binary_modulo_by<meter_int, int>);
   static_assert(!cat::detail::has_reverse_binary_modulo<meter_int, int>);
   static_assert(!cat::detail::has_modulo_assign<meter_int, meter_int>);
   static_assert(cat::detail::has_binary_modulo_by<cat::int4, cat::int4>);
   static_assert(cat::detail::has_modulo_assign<cat::int4, cat::int4>);
   static_assert(cat::is_constructible<gram_int, kilogram_int>);
   static_assert(cat::is_convertible<kilogram_int, gram_int>);
   static_assert(!cat::is_convertible<gram_int, kilogram_int>);
   static_assert(
      cat::is_convertible<cat::units::degree_float8, cat::units::radian_float8>
   );
   static_assert(
      cat::is_convertible<cat::units::radian_float8, cat::units::degree_float8>
   );
   static_assert(
      !cat::is_convertible<cat::units::degree_int8, cat::units::radian_int8>
   );
   static_assert(
      cat::is_convertible<cat::units::minute_int4, cat::units::second_int8>
   );
   static_assert(
      !cat::is_convertible<cat::units::second_int4, cat::units::minute_int8>
   );
   static_assert(!cat::is_constructible<meter_int, second_int>);
   static_assert(!cat::detail::has_binary_plus<item_int, damaged_item_int>);
   static_assert(
      cat::is_quantity_specifier<__typeof_unqual(cat::units::metre)>
   );
   static_assert(cat::is_unit_specifier<__typeof_unqual(cat::units::metre)>);
   static_assert(cat::is_quantity_specifier<__typeof_unqual(width_in_metres)>);
   static_assert(!cat::is_unit_specifier<__typeof_unqual(width_in_metres)>);
   static_assert(
      displacement_spec::character.order == cat::quantity_tensor_order::vector
   );
   static_assert(
      cat::is_same<horizontal_area_spec::parent_type, __typeof_unqual(area)>
   );
   static_assert(
      cat::is_same<
         cat::reference_quantity<cat::units::kilogram>, cat::units::kilograms>
   );
   static_assert(meter_int(0) == 0);
   static_assert(0 == meter_int(0));

   constexpr auto acceleration = meter_int(12) / second_int(2) / second_int(3);
   static_assert(cat::is_same<
                 decltype(acceleration)::quantity_type,
                 cat::units::metres_per_second_squared>);
   static_assert(acceleration.raw == 2);

   constexpr auto scalar = meter_int(12) / meter_int(3);
   static_assert(
      cat::is_dimensionless_quantity<decltype(scalar)::quantity_type>
   );
   static_assert(scalar.raw == 4);

   constexpr gram_int grams = kilogram_int(2);
   static_assert(grams.raw == 2'000);
   constexpr auto radians_parameter = [](cat::units::radian_float8 value) {
      return value.raw;
   };
   static_assert(
      cat::abs(
         radians_parameter(cat::units::degree_float8(180.0)) - cat::pi<double>
      )
      < 1e-12
   );
   constexpr cat::units::degree_float8 converted_degrees =
      cat::units::radian_float8(cat::pi<double>);
   static_assert(cat::abs(converted_degrees.raw - 180.0) < 1e-12);

   gram_int assigned_grams;
   assigned_grams = kilogram_int(3);
   cat::verify(assigned_grams.raw == 3'000);

   cat::units::radian_float8 assigned_radians;
   assigned_radians = cat::units::degree_float8(180.0);
   cat::verify(cat::abs(assigned_radians.raw - cat::pi<double>) < 1e-12);

   constexpr cat::units::gram_wrap_int1 wrapped_grams =
      cat::units::kilogram_int1(1);
   constexpr cat::units::gram_sat_int1 saturated_grams =
      cat::units::kilogram_int1(1);
   static_assert(wrapped_grams.raw == -24);
   static_assert(saturated_grams.raw == cat::int1_max);

   constexpr auto combined_mass = kilogram_int(1) + gram_int(1);
   static_assert(
      cat::is_same<decltype(combined_mass)::quantity_type, cat::units::grams>
   );
   static_assert(combined_mass.raw == 1'001);

   constexpr auto speed =
      (10.0 * cat::units::metre) / (4.0 * cat::units::second);
   static_assert(
      cat::is_same<
         decltype(speed)::quantity_type, cat::units::metres_per_second>
   );
   static_assert(speed.raw == 2.5);
   static_assert(
      cat::is_same<
         cat::reference_quantity<cat::units::metre / cat::units::second>,
         cat::units::metres_per_second>
   );
   static_assert(
      cat::is_same<
         cat::multiplied_quantity<cat::units::metres, cat::units::seconds>,
         cat::multiplied_quantity<cat::units::seconds, cat::units::metres>>
   );

   constexpr item_int item_count_value(6);
   constexpr auto item_rate = item_count_value / second_int(2);
   using time_spec = __typeof_unqual(cat::units::time);
   static_assert(
      cat::is_same<
         decltype(item_rate)::quantity_type::dimension_type,
         cat::divided_dimension<item_dimension_type, time_spec::dimension_type>>
   );
   static_assert(item_rate.raw == 3);

   constexpr width_int width_value(2);
   constexpr auto length_sum = width_value + meter_int(3);
   static_assert(length_sum.raw == 5);
   constexpr height_int height_value(3);
   constexpr auto rectangular_sum = width_value + height_value;
   static_assert(rectangular_sum.raw == 5);
   static_assert(cat::is_same<
                 decltype(rectangular_sum)::quantity_type::quantity_spec_type,
                 __typeof_unqual(cat::units::length)>);

   constexpr double_metre_int doubled_unit_value(3);
   constexpr meter_int converted_double_metres = doubled_unit_value;
   static_assert(converted_double_metres.raw == 6);

   constexpr second_int minute_seconds = minute_int(2);
   static_assert(minute_seconds.raw == 120);
   constexpr cat::units::second_int8 widened_minute_seconds =
      cat::units::minute_int4(2);
   static_assert(widened_minute_seconds.raw == 120);
   static_assert(minute != cat::units::second);
   static_assert(cat::is_equivalent(minute, cat::mag<60> * cat::units::second));

   static_assert(cat::is_same<
                 cat::multiplied_dimension<
                    item_dimension_type, duplicate_item_dimension_type>,
                 cat::multiplied_dimension<
                    duplicate_item_dimension_type, item_dimension_type>>);

   constexpr auto root_metre = cat::sqrt(cat::units::metre);
   using root_metre_quantity = cat::reference_quantity<root_metre>;
   using root_metre_unit = root_metre_quantity::unit_type;
   static_assert(
      cat::is_same<
         root_metre_unit,
         cat::unit<cat::unit_power<__typeof_unqual(cat::units::metre), 1, 2>>>
   );

   constexpr auto squared_metre = cat::pow<2>(cat::units::metre);
   using squared_metre_quantity = cat::reference_quantity<squared_metre>;
   static_assert(
      cat::is_same<
         squared_metre_quantity::unit_type,
         cat::unit<cat::unit_power<__typeof_unqual(cat::units::metre), 2>>>
   );

   constexpr auto cube_root_metre = cat::cbrt(cat::units::metre);
   using cube_root_metre_quantity = cat::reference_quantity<cube_root_metre>;
   static_assert(
      cat::is_same<
         cube_root_metre_quantity::unit_type,
         cat::unit<cat::unit_power<__typeof_unqual(cat::units::metre), 1, 3>>>
   );

   constexpr auto inverse_metre = cat::inverse(cat::units::metre);
   using inverse_metre_reference = cat::reference_quantity<inverse_metre>;
   static_assert(
      cat::is_same<
         inverse_metre_reference::unit_type,
         cat::unit<cat::unit_power<__typeof_unqual(cat::units::metre), -1>>>
   );

   static_assert(cat::is_same<
                 decltype(meter_int(1).wrap()),
                 cat::basic_int<
                    cat::int8::raw_type, cat::overflow_policies::wrap,
                    cat::units::metre>>);
   static_assert(cat::is_same<
                 decltype(meter_float(1.0).fast()),
                 cat::basic_float<
                    cat::float8::raw_type, cat::precision_policies::fast,
                    cat::units::metre>>);
   static_assert(__is_trivial(meter_int));
   static_assert(sizeof(meter_int) == sizeof(meter_int::raw_type));
   static_assert(sizeof(meter_float) == sizeof(meter_float::raw_type));

   using meter_vector = cat::fixed_size_simd<meter_int, 4u>;
   using second_vector = cat::fixed_size_simd<second_int, 4u>;
   using raw_lane = meter_int::raw_type;

   static_assert(sizeof(meter_vector) == sizeof(meter_vector::raw_type));
   static_assert(can_load<meter_vector, meter_int>);
   static_assert(!can_load<meter_vector, raw_lane>);
   static_assert(!can_cast<meter_vector, second_vector>);
   static_assert(!can_reduce_mul<meter_vector>);

   meter_int meter_lanes[] = {
      meter_int(1),
      meter_int(2),
      meter_int(3),
      meter_int(4),
   };
   meter_int stored_lanes[] = {
      meter_int(0),
      meter_int(0),
      meter_int(0),
      meter_int(0),
   };
   meter_vector meters;
   meters.load(meter_lanes);
   meters.store(stored_lanes);
   for (cat::idx i = 0u; i < 4u; ++i) {
      cat::verify(stored_lanes[i] == meter_lanes[i]);
   }

   second_vector seconds(
      second_int(1), second_int(1), second_int(1), second_int(1)
   );
   auto const vector_acceleration = meters / seconds / seconds;
   static_assert(cat::is_same<
                 decltype(vector_acceleration)::value_type::quantity_type,
                 cat::units::metres_per_second_squared>);
   for (cat::idx i = 0u; i < 4u; ++i) {
      cat::verify(vector_acceleration[i].raw == meter_lanes[i].raw);
   }

   using item_vector = cat::fixed_size_simd<item_int, 4u>;
   item_vector item_counts(item_int(2), item_int(4), item_int(6), item_int(8));
   auto const item_rates = item_counts / seconds;
   static_assert(cat::is_same<
                 decltype(item_rates)::value_type::quantity_type,
                 cat::divided_quantity<items, cat::units::seconds>>);

   auto const doubled_meters = meters * 2;
   static_assert(cat::is_same<
                 decltype(doubled_meters)::value_type::quantity_type,
                 cat::units::metres>);
   auto const square_meters = meters * meters;
   static_assert(
      cat::is_same<
         decltype(square_meters)::value_type::quantity_type,
         cat::multiplied_quantity<cat::units::metres, cat::units::metres>>
   );
   auto const wrapped_sum = meters.wrap() + meters;
   static_assert(
      cat::is_same<
         decltype(wrapped_sum)::value_type::quantity_type, cat::units::metres>
   );

   using meter_float_vector = cat::fixed_size_simd<meter_float, 4u>;
   meter_float_vector float_meters(
      meter_float(1.0), meter_float(2.0), meter_float(3.0), meter_float(4.0)
   );
   auto const fast_sum = float_meters.fast() + float_meters;
   static_assert(
      decltype(fast_sum)::value_type::precision_policy
      == cat::precision_policies::fast
   );
   static_assert(
      cat::is_same<
         decltype(fast_sum)::value_type::quantity_type, cat::units::metres>
   );

   using degree_vector = cat::fixed_size_simd<cat::units::degree_float8, 4u>;
   using radian_vector = cat::fixed_size_simd<cat::units::radian_float8, 4u>;
   degree_vector const degree_angles(
      cat::units::degree_float8(0.0), cat::units::degree_float8(90.0),
      cat::units::degree_float8(180.0), cat::units::degree_float8(360.0)
   );
   radian_vector const radian_angles = degree_angles;
   radian_vector const broadcast_radians = cat::units::degree_float8(180.0);
   cat::verify(cat::abs(radian_angles[1].raw - cat::pi<double> / 2) < 1e-12);
   cat::verify(cat::abs(broadcast_radians[0].raw - cat::pi<double>) < 1e-12);

   auto const mask =
      meters
      > meter_vector(meter_int(0), meter_int(1), meter_int(2), meter_int(3));
   cat::verify(mask.all_of());
   cat::verify(meters.sum().raw == 10);

   cat::span page = pager.alloc_multi<cat::byte>(4_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::verify(cat::fmt(allocator, "{}", kilogram_int(12)).verify() == "12 kg");
   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{}", cat::units::standard_atmosphere).verify()
      == "101325 Pa"
   );
   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{}", 9.8 * cat::units::metre_per_second_squared)
         .verify()
      == "9.8E0 m/s^2"
   );
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", scalar).verify() == "4");
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", item_rate).verify() == "3 item/s");

   meter_int overflow_value(7);
   cat::verify(
      cat::fmt(allocator, "{}", overflow_value.wrap()).verify() == "7 m"
   );
   allocator.reset();
   meter_float precision_value(1.5);
   cat::verify(
      cat::fmt(allocator, "{}", precision_value.fast()).verify() == "1.5E0 m"
   );
}

$test(quantity_time_utc_leap_second) {
   constexpr auto before = cat::utc_from_date_time({
      .year = 2'016,
      .month = 12,
      .day = 31,
      .hour = 23,
      .minute = 59,
      .second = 59,
   });
   constexpr auto leap = cat::utc_from_date_time({
      .year = 2'016,
      .month = 12,
      .day = 31,
      .hour = 23,
      .minute = 59,
      .second = 60,
   });
   constexpr auto after = cat::utc_from_date_time({
      .year = 2'017,
      .month = 1,
      .day = 1,
      .hour = 0,
      .minute = 0,
      .second = 0,
   });

   static_assert(before.valid);
   static_assert(leap.valid);
   static_assert(after.valid);
   static_assert(leap.value - before.value == cat::duration(1));
   static_assert(after.value - leap.value == cat::duration(1));
   static_assert(after.value - before.value == cat::duration(2));
   static_assert(cat::date_time_from_utc(leap.value).second == 60);
   static_assert(cat::get_leap_second_info(leap.value).is_leap_second);
   static_assert(
      cat::get_leap_second_info(leap.value).elapsed == cat::duration(27)
   );
   static_assert(
      cat::leap_seconds[26].date.seconds_since_epoch()
      == cat::duration(1'483'228'800)
   );
}

$test(quantity_time_utc_validation_and_unix_mapping) {
   constexpr cat::utc_date_time invalid = {
      .year = 2'017,
      .month = 12,
      .day = 31,
      .hour = 23,
      .minute = 59,
      .second = 60,
   };
   static_assert(!cat::is_valid_utc(invalid));

   constexpr auto leap = cat::utc_from_date_time({
      .year = 2'016,
      .month = 12,
      .day = 31,
      .hour = 23,
      .minute = 59,
      .second = 60,
   });
   constexpr auto previous =
      cat::to_unix(leap.value, cat::unix_leap_second_mapping::previous_second);
   constexpr auto next =
      cat::to_unix(leap.value, cat::unix_leap_second_mapping::next_second);
   static_assert(next - previous == cat::duration(1));
   static_assert(cat::to_utc(next) == leap.value + cat::duration(1));

   constexpr cat::utc_time_point utc_epoch(cat::duration(0));
   constexpr auto tai_epoch_coordinate = cat::to_tai(utc_epoch);
   static_assert(
      tai_epoch_coordinate.seconds_since_epoch() == cat::duration(378'691'210)
   );
   static_assert(cat::to_utc(tai_epoch_coordinate) == utc_epoch);
}
