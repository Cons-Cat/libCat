#include <cat/math>

#include "../unit_tests.hpp"
#include "cat/debug"

TEST(test_math) {
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
    cat::verify(cat::abs(1_i8) == 1_i8);

    cat::verify(cat::abs(1.f) == 1.f);
    cat::verify(cat::abs(1.) == 1.);
    cat::verify(cat::abs(-1) == 1);
    cat::verify(cat::abs(-1.f) == 1.f);
    cat::verify(cat::abs(-1.) == 1.);

    cat::verify(cat::abs(1u) == 1);
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

    // Test `is_power_of_two()`.
    cat::verify(cat::is_power_of_two(0));
    cat::verify(cat::is_power_of_two(1));
    cat::verify(cat::is_power_of_two(2));
    cat::verify(!cat::is_power_of_two(3));
    cat::verify(cat::is_power_of_two(4));
    cat::verify(cat::is_power_of_two(8u));
    cat::verify(cat::is_power_of_two(256));

    // Test `round_to_pow2()`.
    cat::round_to_pow2(0u);
    static_assert(cat::round_to_pow2(0u) == 0u);
    static_assert(cat::round_to_pow2(1u) == 1u);
    static_assert(cat::round_to_pow2(2u) == 2u);
    static_assert(cat::round_to_pow2(3u) == 4u);
    static_assert(cat::round_to_pow2(4u) == 4u);
    static_assert(cat::round_to_pow2(5u) == 8u);

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
