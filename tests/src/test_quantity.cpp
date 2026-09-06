#include <cat/format>
#include <cat/linear_allocator>
#include <cat/math>
#include <cat/page_allocator>
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

using meter_int = quantity_int<cat::si::metre>;
using meter_uint = quantity_uint<cat::si::metre>;
using second_int = quantity_int<cat::si::second>;
using gram_int = quantity_int<cat::si::gram>;
using kilogram_int = quantity_int<cat::si::kilogram>;
using meter_float = quantity_float<cat::si::metre>;
using second_float = quantity_float<cat::si::second>;
inline constexpr auto kilometre = cat::si::kilo<cat::si::metre>;
using kilometre_float = quantity_float<kilometre>;

inline constexpr auto metre_second = cat::si::metre * cat::si::second;
inline constexpr auto square_metre = cat::pow<2>(cat::si::metre);
inline constexpr auto cubic_metre = cat::pow<3>(cat::si::metre);
using metre_second_int = quantity_int<metre_second>;
using metre_second_float = quantity_float<metre_second>;
using square_metre_float = quantity_float<square_metre>;
using cubic_metre_float = quantity_float<cubic_metre>;

inline constexpr struct ratio : cat::quantity_spec<cat::dimension<>{}> {
} ratio;

using ratio_quantity =
   cat::quantity_reference<__typeof_unqual(ratio), cat::unit<>>;
inline constexpr ratio_quantity ratio_reference;
using ratio_float = quantity_float<ratio_reference>;

struct item_dimension : cat::base_dimension<"item"> {};

using item_dimension_type =
   cat::dimension<cat::dimension_power<item_dimension, 1>>;

struct item_count : cat::quantity_spec<item_dimension_type{}> {};

struct item : cat::named_unit<"item", item_count{}> {};

using items =
   cat::quantity_reference<item_count, cat::unit<cat::unit_power<item, 1>>>;

struct damaged_item_count : cat::quantity_spec<item_dimension_type{}> {};

using damaged_items = cat::quantity_reference<
   damaged_item_count, cat::unit<cat::unit_power<item, 1>>>;
inline constexpr items item_reference;
inline constexpr damaged_items damaged_item_reference;
using item_int = quantity_int<item_reference>;
using damaged_item_int = quantity_int<damaged_item_reference>;

inline constexpr struct width : cat::quantity_spec<cat::isq::length> {
} width;

inline constexpr struct height : cat::quantity_spec<cat::isq::length> {
} height;

inline constexpr struct area
    : cat::quantity_spec<cat::pow<2>(cat::isq::length)> {
} area;

inline constexpr struct horizontal_area
    : cat::quantity_spec<area, width * height> {
} horizontal_area;

using horizontal_area_spec = __typeof_unqual(horizontal_area);

inline constexpr auto width_in_metres = width[cat::si::metre];
inline constexpr auto height_in_metres = height[cat::si::metre];
using width_int = quantity_int<width_in_metres>;
using height_int = quantity_int<height_in_metres>;

inline constexpr struct displacement
    : cat::quantity_spec<
         cat::isq::dim_length, cat::quantity_character{
                                  cat::quantity_tensor_order::vector,
                                  cat::quantity_field::real,
                               }> {
} displacement;

using displacement_spec = __typeof_unqual(displacement);

inline constexpr struct double_metre
    : cat::prefixed_unit<"2m", cat::mag_ratio<2>, cat::si::metre> {
} double_metre;

using double_metre_int = quantity_int<double_metre>;

inline constexpr struct minute
    : cat::named_unit<"min", cat::mag<60> * cat::si::second> {
} minute;

using minute_int = quantity_int<minute>;

struct duplicate_item_dimension : cat::base_dimension<"item"> {};

using duplicate_item_dimension_type =
   cat::dimension<cat::dimension_power<duplicate_item_dimension, 1>>;

template <typename Left, typename Right>
concept can_add = requires(Left left, Right right) { left + right; };

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
                                     cat::sin(value);
                                     cat::cos(value);
                                     cat::tan(value);
                                     cat::asin(value);
                                     cat::acos(value);
                                     cat::atan(value);
                                     cat::sinh(value);
                                     cat::cosh(value);
                                     cat::tanh(value);
                                     cat::lgamma(value);
                                  };

template <typename T>
concept has_runtime_power = requires(T value) { cat::pow(value, 2); };

template <typename T>
concept has_runtime_root = requires(T value) { cat::nroot(value, 2); };

template <typename T, typename U, typename V>
concept can_fma = requires(T value, U multiplier, V addend) {
                     cat::fma(value, multiplier, addend);
                  };

}  // namespace

$test(quantity_math_preserving) {
   constexpr meter_int negative_metres(-5);
   static_assert(cat::abs(negative_metres) == 5_m);
   static_assert(cat::is_same<decltype(cat::abs(negative_metres)), meter_int>);

   constexpr meter_float fractional_metres(2.75);
   static_assert(cat::floor(fractional_metres).raw == 2.0);
   static_assert(cat::ceil(fractional_metres).raw == 3.0);
   static_assert(
      cat::is_same<decltype(cat::floor(fractional_metres)), meter_float>
   );

   static_assert(cat::min(3_m, 2_m) == 2_m);
   static_assert(cat::max(3_m, 2_m) == 3_m);
   static_assert(cat::clamp(12_m, 0_m, 10_m) == 10_m);
   static_assert(cat::sum(1_m, 2_m, 3_m) == 6_m);

   static_assert(cat::is_less(1_m, 2_m));
   static_assert(cat::is_greater(2_m, 1_m));
   static_assert(cat::is_less_equal(2_m, 2_m));
   static_assert(cat::is_greater_equal(2_m, 2_m));
   static_assert(cat::is_equal_to(2_m, 2_m));
   static_assert(cat::is_not_equal_to(1_m, 2_m));
   static_assert(cat::compare_three_way(1_m, 2_m) < 0);
   static_assert(cat::is_less(1_m)(2_m));
}

$test(quantity_math_dimension_algebra) {
   constexpr auto product = cat::product(2_m, 3_s);
   static_assert(product.raw == 6);
   static_assert(cat::is_same<
                 decltype(product)::quantity_type,
                 cat::reference_quantity<metre_second>>);

   constexpr auto dot = cat::dot(2_m, 3_s, 4_m, 5_s);
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
      cat::is_same<side_quantity::unit_type, cat::si::meters::unit_type>
   );
   static_assert(
      cat::is_same<
         side_quantity::dimension_type, cat::si::meters::dimension_type>
   );

   constexpr auto reciprocal_side = cat::rsqrt(area_value);
   static_assert(reciprocal_side.raw == 1.0 / 3.0);
   using reciprocal_side_quantity = decltype(reciprocal_side)::quantity_type;
   using inverse_metre_quantity =
      cat::reference_quantity<cat::inverse(cat::si::metre)>;
   static_assert(
      cat::is_same<
         reciprocal_side_quantity::unit_type, inverse_metre_quantity::unit_type>
   );

   constexpr cubic_metre_float volume_value(27.0);
   constexpr auto edge = cat::cbrt(volume_value);
   static_assert(edge.raw == 3.0);
   using edge_quantity = decltype(edge)::quantity_type;
   static_assert(
      cat::is_same<edge_quantity::unit_type, cat::si::meters::unit_type>
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
      cat::is_same<rooted_quantity::unit_type, cat::si::meters::unit_type>
   );

   static_assert(!has_runtime_power<meter_float>);
   static_assert(!has_runtime_root<meter_float>);
}

$test(quantity_math_fma_and_division) {
   constexpr auto integer_fma = cat::fma(2_m, 3_s, metre_second_int(4));
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
      cat::is_same<decltype(fast_fma)::quantity_type, cat::si::meters>
   );

   constexpr auto converted_fma =
      cat::fma(meter_float(2.0), 3.0, kilometre_float(1.0));
   static_assert(converted_fma.raw == 1'006.0);

   constexpr auto integer_floor = cat::div_floor(5_m, 2_s);
   constexpr auto integer_ceil = cat::div_ceil(5_m, 2_s);
   static_assert(integer_floor.raw == 2);
   static_assert(integer_ceil.raw == 3);
   static_assert(
      cat::is_same<
         decltype(integer_floor)::quantity_type, cat::si::meters_per_second>
   );

   constexpr auto float_floor =
      cat::div_floor(meter_float(5.0), second_float(2.0));
   constexpr auto float_ceil =
      cat::div_ceil(meter_float(5.0), second_float(2.0));
   static_assert(float_floor.raw == 2.0);
   static_assert(float_ceil.raw == 3.0);
   static_assert(
      cat::is_same<
         decltype(float_floor)::quantity_type, cat::si::meters_per_second>
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
   static_assert(cat::sin(zero).raw == 0.0);
   static_assert(cat::cos(zero).raw == 1.0);
   static_assert(cat::tan(zero).raw == 0.0);
   static_assert(cat::asin(zero).raw == 0.0);
   static_assert(cat::acos(one).raw == 0.0);
   static_assert(cat::atan(zero).raw == 0.0);
   static_assert(cat::sinh(zero).raw == 0.0);
   static_assert(cat::cosh(zero).raw == 1.0);
   static_assert(cat::tanh(zero).raw == 0.0);
   static_assert(cat::abs(cat::lgamma(one).raw) < 1e-12);

   constexpr auto angle =
      cat::atan2(meter_float(1'000.0), kilometre_float(1.0));
   static_assert(
      cat::is_dimensionless_quantity<decltype(angle)::quantity_type>
   );
   static_assert(cat::abs(angle.raw - (cat::pi<double> / 4.0)) < 1e-12);

   static_assert(cat::is_even(4_m));
   static_assert(cat::is_odd(3_m));
   static_assert(cat::is_divisible_by(12_m, 3_m));
   static_assert(
      cat::round_up_to_multiple_of(meter_uint(5), meter_uint(2))
      == meter_uint(6)
   );
   static_assert(cat::round_down_to_multiple_of(5_m, 2_m) == 4_m);
   static_assert(cat::round_to_multiple_of(5_m, 2_m) == 6_m);

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
   static_assert(cat::is_same<decltype(1_m), meter_int>);
   static_assert(cat::is_same<decltype(1_s), second_int>);
   static_assert(cat::is_same<decltype(1_g), gram_int>);
   static_assert(cat::is_same<decltype(1_kg), kilogram_int>);
   static_assert(
      cat::is_same<
         decltype(1_ms2), quantity_int<cat::si::metre_per_second_squared>>
   );
   static_assert(cat::is_same<decltype(1.0_m), meter_float>);

   static_assert(!can_add<meter_int, second_int>);
   static_assert(cat::is_constructible<gram_int, kilogram_int>);
   static_assert(!cat::is_convertible<kilogram_int, gram_int>);
   static_assert(!cat::is_constructible<meter_int, second_int>);
   static_assert(!can_add<item_int, damaged_item_int>);
   static_assert(cat::is_quantity_specifier<__typeof_unqual(cat::si::metre)>);
   static_assert(cat::Unit<__typeof_unqual(cat::si::metre)>);
   static_assert(cat::is_quantity_specifier<__typeof_unqual(width_in_metres)>);
   static_assert(!cat::Unit<__typeof_unqual(width_in_metres)>);
   static_assert(
      displacement_spec::character.order == cat::quantity_tensor_order::vector
   );
   static_assert(
      cat::is_same<horizontal_area_spec::parent_type, __typeof_unqual(area)>
   );
   static_assert(
      cat::is_same<
         cat::reference_quantity<cat::si::kilogram>, cat::si::kilograms>
   );
   static_assert(0_m == 0);
   static_assert(0 == 0_m);

   constexpr auto acceleration = 12_m / 2_s / 3_s;
   static_assert(cat::is_same<
                 decltype(acceleration)::quantity_type,
                 cat::si::meters_per_second_squared>);
   static_assert(acceleration.raw == 2);

   constexpr auto scalar = 12_m / 3_m;
   static_assert(
      cat::is_dimensionless_quantity<decltype(scalar)::quantity_type>
   );
   static_assert(scalar.raw == 4);

   constexpr gram_int grams(2_kg);
   static_assert(grams.raw == 2'000);

   constexpr auto combined_mass = 1_kg + 1_g;
   static_assert(
      cat::is_same<decltype(combined_mass)::quantity_type, cat::si::grams>
   );
   static_assert(combined_mass.raw == 1'001);

   constexpr auto speed = 10.0_m / 4.0_s;
   static_assert(
      cat::is_same<decltype(speed)::quantity_type, cat::si::meters_per_second>
   );
   static_assert(speed.raw == 2.5);
   static_assert(cat::is_same<
                 cat::reference_quantity<cat::si::metre / cat::si::second>,
                 cat::si::meters_per_second>);
   static_assert(cat::is_same<
                 cat::multiplied_quantity<cat::si::meters, cat::si::seconds>,
                 cat::multiplied_quantity<cat::si::seconds, cat::si::meters>>);

   constexpr item_int item_count_value(6);
   constexpr auto item_rate = item_count_value / 2_s;
   using time_spec = __typeof_unqual(cat::isq::time);
   static_assert(
      cat::is_same<
         decltype(item_rate)::quantity_type::dimension_type,
         cat::divided_dimension<item_dimension_type, time_spec::dimension_type>>
   );
   static_assert(item_rate.raw == 3);

   constexpr width_int width_value(2);
   constexpr auto length_sum = width_value + 3_m;
   static_assert(length_sum.raw == 5);
   constexpr height_int height_value(3);
   constexpr auto rectangular_sum = width_value + height_value;
   static_assert(rectangular_sum.raw == 5);
   static_assert(cat::is_same<
                 decltype(rectangular_sum)::quantity_type::quantity_spec_type,
                 __typeof_unqual(cat::isq::length)>);

   constexpr double_metre_int doubled_unit_value(3);
   constexpr meter_int converted_double_metres(doubled_unit_value);
   static_assert(converted_double_metres.raw == 6);

   constexpr second_int minute_seconds(minute_int(2));
   static_assert(minute_seconds.raw == 120);
   static_assert(minute != cat::si::second);
   static_assert(cat::equivalent(minute, cat::mag<60> * cat::si::second));

   static_assert(cat::is_same<
                 cat::multiplied_dimension<
                    item_dimension_type, duplicate_item_dimension_type>,
                 cat::multiplied_dimension<
                    duplicate_item_dimension_type, item_dimension_type>>);

   constexpr auto root_metre = cat::sqrt(cat::si::metre);
   using root_metre_quantity = cat::reference_quantity<root_metre>;
   using root_metre_unit = root_metre_quantity::unit_type;
   static_assert(
      cat::is_same<
         root_metre_unit,
         cat::unit<cat::unit_power<__typeof_unqual(cat::si::metre), 1, 2>>>
   );

   constexpr auto squared_metre = cat::pow<2>(cat::si::metre);
   using squared_metre_quantity = cat::reference_quantity<squared_metre>;
   static_assert(
      cat::is_same<
         squared_metre_quantity::unit_type,
         cat::unit<cat::unit_power<__typeof_unqual(cat::si::metre), 2>>>
   );

   constexpr auto cube_root_metre = cat::cbrt(cat::si::metre);
   using cube_root_metre_quantity = cat::reference_quantity<cube_root_metre>;
   static_assert(
      cat::is_same<
         cube_root_metre_quantity::unit_type,
         cat::unit<cat::unit_power<__typeof_unqual(cat::si::metre), 1, 3>>>
   );

   constexpr auto inverse_metre = cat::inverse(cat::si::metre);
   using inverse_metre_reference = cat::reference_quantity<inverse_metre>;
   static_assert(
      cat::is_same<
         inverse_metre_reference::unit_type,
         cat::unit<cat::unit_power<__typeof_unqual(cat::si::metre), -1>>>
   );

   static_assert(
      cat::is_same<
         decltype(meter_int(1).wrap()),
         cat::basic_int<
            cat::int8::raw_type, cat::overflow_policies::wrap, cat::si::metre>>
   );
   static_assert(cat::is_same<
                 decltype(meter_float(1.0).fast()),
                 cat::basic_float<
                    cat::float8::raw_type, cat::precision_policies::fast,
                    cat::si::metre>>);
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

   meter_int meter_lanes[] = {1_m, 2_m, 3_m, 4_m};
   meter_int stored_lanes[] = {0_m, 0_m, 0_m, 0_m};
   meter_vector meters;
   meters.load(meter_lanes);
   meters.store(stored_lanes);
   for (cat::idx i = 0u; i < 4u; ++i) {
      cat::verify(stored_lanes[i] == meter_lanes[i]);
   }

   second_vector seconds(1_s, 1_s, 1_s, 1_s);
   auto const vector_acceleration = meters / seconds / seconds;
   static_assert(cat::is_same<
                 decltype(vector_acceleration)::value_type::quantity_type,
                 cat::si::meters_per_second_squared>);
   for (cat::idx i = 0u; i < 4u; ++i) {
      cat::verify(vector_acceleration[i].raw == meter_lanes[i].raw);
   }

   using item_vector = cat::fixed_size_simd<item_int, 4u>;
   item_vector item_counts(item_int(2), item_int(4), item_int(6), item_int(8));
   auto const item_rates = item_counts / seconds;
   static_assert(cat::is_same<
                 decltype(item_rates)::value_type::quantity_type,
                 cat::divided_quantity<items, cat::si::seconds>>);

   auto const doubled_meters = meters * 2;
   static_assert(
      cat::is_same<
         decltype(doubled_meters)::value_type::quantity_type, cat::si::meters>
   );
   auto const square_meters = meters * meters;
   static_assert(cat::is_same<
                 decltype(square_meters)::value_type::quantity_type,
                 cat::multiplied_quantity<cat::si::meters, cat::si::meters>>);
   auto const wrapped_sum = meters.wrap() + meters;
   static_assert(
      cat::is_same<
         decltype(wrapped_sum)::value_type::quantity_type, cat::si::meters>
   );

   using meter_float_vector = cat::fixed_size_simd<meter_float, 4u>;
   meter_float_vector float_meters(1.0_m, 2.0_m, 3.0_m, 4.0_m);
   auto const fast_sum = float_meters.fast() + float_meters;
   static_assert(
      decltype(fast_sum)::value_type::precision_policy
      == cat::precision_policies::fast
   );
   static_assert(
      cat::is_same<
         decltype(fast_sum)::value_type::quantity_type, cat::si::meters>
   );

   auto const mask = meters > meter_vector(0_m, 1_m, 2_m, 3_m);
   cat::verify(mask.all_of());
   cat::verify(meters.sum().raw == 10);

   cat::span page = pager.alloc_multi<cat::byte>(4_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = make_linear_allocator(page);

   cat::verify(cat::fmt(allocator, "{}", 12_kg).verify() == "12 kg");
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", 9.8_ms2).verify() == "9.8E0 m/s^2");
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", scalar).verify() == "4");
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", item_rate).verify() == "3 item/s");

   meter_int overflow_value = 7_m;
   cat::verify(
      cat::fmt(allocator, "{}", overflow_value.wrap()).verify() == "7 m"
   );
   allocator.reset();
   meter_float precision_value = 1.5_m;
   cat::verify(
      cat::fmt(allocator, "{}", precision_value.fast()).verify() == "1.5E0 m"
   );
}
