#include <cat/math>

#include "../unit_tests.hpp"
#include "cat/debug"

test(math) {
   using namespace cat::arithmetic_literals;

   // Test `min()`.
   cat::verify(cat::min(0) == 0);
   cat::verify(cat::min(0u) == 0u);
   cat::verify(cat::min(0.f) == 0.f);
   cat::verify(cat::min(0.) == 0.);

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

   cat::verify(cat::max(0, 1) == 1);
   cat::verify(cat::max(0u, 1u) == 1u);
   cat::verify(cat::max(0.f, 1.f) == 1.f);
   cat::verify(cat::max(0., 1.) == 1.);

   cat::verify(cat::max(0, 1, 2) == 2);
   cat::verify(cat::max(0u, 1u, 2u) == 2u);
   cat::verify(cat::max(0.f, 1.f, 2.f) == 2.f);
   cat::verify(cat::max(0., 1., 2.) == 2.);

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

   static_assert(cat::is_same<int, typeof(cat::abs(1))>);
   static_assert(cat::is_same<float, typeof(cat::abs(1.f))>);
   static_assert(cat::is_same<double, typeof(cat::abs(1.))>);

   cat::verify(cat::abs(1u) == 1);
   cat::verify(cat::abs(1_u1) == 1_u1);
   cat::verify(cat::abs(1_u2) == 1_u2);
   cat::verify(cat::abs(1_u4) == 1_u4);
   cat::verify(cat::abs(1_u8) == 1_u8);

   // Test `pow()`.
   cat::verify(cat::pow(2, 2) == 4);
   cat::verify(cat::pow(2, 1) == 2);
   cat::verify(cat::pow(2, 0) == 1);
   cat::verify(cat::pow(1, 10) == 1);
   cat::verify(cat::pow(8, -1) == 0);

   cat::verify(cat::pow(2u, 2u) == 4u);

   // cat::verify(cat::pow(2.f, 2) == 4.f);
   // cat::verify(cat::pow(2.f, 1) == 2.f);
   // cat::verify(cat::pow(2.f, 0) == 1.f);
   // cat::verify(cat::pow(8.f, -1) == 0.f);

   // cat::verify(cat::pow(2, 2) == 4.);
   // cat::verify(cat::pow(2, 1) == 2.);
   // cat::verify(cat::pow(2, 0) == 1.);
   // cat::verify(cat::pow(8, -1) == 0.);

   // TODO: How should rounding functions handle negative inputs?

   // Test `round_up_to_multiple_of()`.
   static_assert(cat::round_up_to_multiple_of(5, 2) == 6);
   static_assert(cat::round_up_to_multiple_of(5u, 1) == 5u);
   static_assert(cat::round_up_to_multiple_of(5, 8) == 8);

   // Test `round_down_to_multiple_of()`.
   static_assert(cat::round_down_to_multiple_of(5, 2) == 4);
   static_assert(cat::round_down_to_multiple_of(5u, 1) == 5u);
   static_assert(cat::round_down_to_multiple_of(5, 8) == 0);

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
