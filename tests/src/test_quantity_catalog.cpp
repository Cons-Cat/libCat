#include <cat/quantity>

#include "../unit_tests.hpp"

namespace {

template <auto quantity>
using quantity_float = cat::basic_float<
   cat::float8::raw_type, cat::precision_policies::precise, quantity>;

using metre_float = quantity_float<cat::units::metre>;

}  // namespace

$test(quantity_catalog_si) {
   using poynting_vector_spec = __typeof_unqual(cat::units::poynting_vector);
   using atomic_scattering_factor_spec =
      __typeof_unqual(cat::units::atomic_scattering_factor);

   static_assert(cat::is_quantity_spec<__typeof_unqual(cat::units::force)>);
   static_assert(cat::is_quantity_spec<
                 __typeof_unqual(cat::units::london_penetration_depth)>);
   static_assert(
      poynting_vector_spec::character.order
      == cat::quantity_tensor_order::vector
   );
   static_assert(
      atomic_scattering_factor_spec::character.field
      == cat::quantity_field::complex
   );
   static_assert(cat::is_unit_specifier<__typeof_unqual(cat::units::newton)>);
   static_assert(
      __is_same(cat::units::newton_int4::raw_type, cat::int4::raw_type)
   );
   static_assert(
      __is_same(cat::units::newton_uint8::raw_type, cat::uint8::raw_type)
   );
   static_assert(
      __is_same(cat::units::newton_float4::raw_type, cat::float4::raw_type)
   );
   static_assert(
      __is_same(cat::units::newton_float8::raw_type, cat::float8::raw_type)
   );
   static_assert(
      cat::make_overflow_policy<cat::units::metre_wrap_int4>
      == cat::overflow_policies::wrap
   );
   static_assert(
      cat::make_overflow_policy<cat::units::metre_sat_uint8>
      == cat::overflow_policies::saturate
   );
   static_assert(
      cat::units::metre_float4_fast::precision_policy
      == cat::precision_policies::fast
   );
   static_assert(__is_same(
      cat::units::metre_per_second_squared_int8::quantity_type,
      cat::units::metres_per_second_squared
   ));
   static_assert(cat::is_equivalent(
      cat::units::newton,
      cat::units::kilogram * cat::units::metre / cat::pow<2>(cat::units::second)
   ));
   static_assert(
      cat::is_equivalent(cat::units::hertz, cat::inverse(cat::units::second))
   );
   static_assert(cat::is_equivalent(
      cat::units::litre, cat::mag_power<10, -3> * cat::units::cubic_metre
   ));
   static_assert(cat::is_equivalent(
      cat::units::degree,
      cat::mag_ratio<1, 180> * cat::mag_pi * cat::units::radian
   ));

   constexpr auto integer_metres = 2 * cat::units::metre;
   constexpr auto floating_metres = cat::units::metre * 2.0;
   constexpr auto inverse_seconds = 2 / cat::units::second;
   static_assert(
      __is_same(__typeof_unqual(integer_metres), cat::units::metre_int4)
   );
   static_assert(
      __is_same(__typeof_unqual(floating_metres), cat::units::metre_float8)
   );
   static_assert(__is_same(
      decltype(inverse_seconds)::quantity_type,
      cat::reference_quantity<cat::inverse(cat::units::second)>
   ));
   static_assert(integer_metres.raw == 2);
   static_assert(floating_metres.raw == 2.0);
   static_assert(inverse_seconds.raw == 2);
}

$test(quantity_catalog_system_conversions) {
   constexpr metre_float inch = cat::units::inch_float8(1.0);
   static_assert(cat::abs(inch.raw - 0.0254) < 1e-12);

   constexpr cat::units::metre_per_second_squared_float8 gal =
      cat::units::gal_float8(1.0);
   static_assert(cat::abs(gal.raw - 0.01) < 1e-12);

   constexpr cat::units::radian_float8 degree = cat::units::degree_float8(1.0);
   static_assert(cat::abs(degree.raw - 0.017'453'292'519'943'295) < 1e-15);
   static_assert(
      cat::is_equivalent(cat::units::astronomical_day, cat::units::day)
   );
   static_assert(cat::units::imperial_gallon != cat::units::us_gallon);
   static_assert(!__is_same(
      __typeof_unqual(cat::units::natural_energy),
      __typeof_unqual(cat::units::energy)
   ));
}

$test(quantity_catalog_pixels) {
   static_assert(
      __is_same(cat::units::pixel_int1::raw_type, cat::int1::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_int2::raw_type, cat::int2::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_int4::raw_type, cat::int4::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_int8::raw_type, cat::int8::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_uint1::raw_type, cat::uint1::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_uint2::raw_type, cat::uint2::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_uint4::raw_type, cat::uint4::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_uint8::raw_type, cat::uint8::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_float4::raw_type, cat::float4::raw_type)
   );
   static_assert(
      __is_same(cat::units::pixel_float8::raw_type, cat::float8::raw_type)
   );
   static_assert(
      cat::units::pixel_float4_fast::precision_policy
      == cat::precision_policies::fast
   );
   static_assert(cat::units::horizontal_pixel_int4(-100).raw == -100);
   static_assert(cat::units::vertical_pixel_uint4(100).raw == 100);
   static_assert(!cat::is_constructible<metre_float, cat::units::pixel_float4>);

   constexpr auto pixels_per_second =
      cat::units::pixel_int4(120) / cat::units::second_int4(2);
   static_assert(cat::is_quantity<decltype(pixels_per_second)::quantity_type>);
   static_assert(__is_same(
      decltype(pixels_per_second)::quantity_type,
      cat::reference_quantity<cat::units::pixel / cat::units::second>
   ));
}

$test(quantity_catalog_alias_coverage) {
   static_assert(cat::units::byte_uint8(8).raw == 8);
   static_assert(cat::units::parsec_float8(1.0).raw == 1.0);
   static_assert(cat::units::gigaelectronvolt_float4(2.0).raw == 2.0);
   static_assert(cat::units::imperial_gallon_float8(3.0).raw == 3.0);
   static_assert(cat::units::us_gallon_float8(4.0).raw == 4.0);
   static_assert(cat::units::didot_point_float4(5.0).raw == 5.0);
   static_assert(cat::units::eplus_float8(6.0).raw == 6.0);
   static_assert(cat::units::nanosecond_int8(7).raw == 7);
}

$test(quantity_catalog_constants) {
   static_assert(cat::units::standard_atmosphere.raw == 101'325);
   static_assert(__is_same(
      decltype(cat::units::standard_atmosphere)::quantity_type,
      cat::reference_quantity<cat::units::pascal>
   ));
   static_assert(cat::units::standard_gravity.raw == 9.806'65);
   static_assert(__is_same(
      decltype(cat::units::standard_gravity)::quantity_type,
      cat::reference_quantity<
         cat::units::metre / cat::pow<2>(cat::units::second)>
   ));
   static_assert(cat::units::avogadro_constant.raw == 6.022'140'76e23);
   static_assert(__is_same(
      decltype(cat::units::avogadro_constant)::quantity_type,
      cat::reference_quantity<cat::inverse(cat::units::mole)>
   ));
   static_assert(
      cat::units::elementary_charge_constant.raw == 1.602'176'634e-19
   );
   static_assert(__is_same(
      decltype(cat::units::elementary_charge_constant)::quantity_type,
      cat::reference_quantity<cat::units::coulomb>
   ));
   static_assert(cat::units::planck_constant.raw == 6.626'070'15e-34);
   static_assert(__is_same(
      decltype(cat::units::planck_constant)::quantity_type,
      cat::reference_quantity<cat::units::joule * cat::units::second>
   ));

   static_assert(
      cat::units::speed_of_light_in_vacuum_constant.raw == 299'792'458
   );
   static_assert(__is_same(
      decltype(cat::units::speed_of_light_in_vacuum_constant)::quantity_type,
      cat::reference_quantity<cat::units::metre / cat::units::second>
   ));

   static_assert(__is_same(
      decltype(cat::units::reduced_planck_constant)::quantity_type,
      cat::reference_quantity<cat::units::joule * cat::units::second>
   ));
   static_assert(
      cat::abs(cat::units::reduced_planck_constant.raw - 1.054'571'817e-34)
      < 1e-43
   );

   static_assert(
      cat::units::fine_structure_constant.raw == 0.007'297'352'564'3
   );
   static_assert(__is_same(
      decltype(cat::units::fine_structure_constant)::quantity_type,
      cat::reference_quantity<cat::units::one>
   ));
   static_assert(
      cat::units::newtonian_constant_of_gravitation.raw == 6.674'30e-11
   );
   static_assert(__is_same(
      decltype(cat::units::newtonian_constant_of_gravitation)::quantity_type,
      cat::reference_quantity<
         cat::pow<3>(cat::units::metre) / cat::units::kilogram
         / cat::pow<2>(cat::units::second)>
   ));
   static_assert(cat::units::molar_gas_constant.raw == 8.314'462'618'153'24);
   static_assert(__is_same(
      decltype(cat::units::molar_gas_constant)::quantity_type,
      cat::reference_quantity<
         cat::units::joule / cat::units::mole / cat::units::kelvin>
   ));

   constexpr metre_float solar_radius(cat::units::nominal_solar_radius);
   static_assert(solar_radius.raw == 695'700'000.0);

   static_assert(cat::is_quantity<cat::units::bytes>);
}
