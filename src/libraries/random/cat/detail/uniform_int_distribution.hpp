// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/distribution_helpers.hpp>
#include <cat/detail/independent_simd_engine.hpp>

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/meta>
#include <cat/random>
#include <cat/simd>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
// https://eel.is/c++draft/rand.dist.uni.int
template <is_integral Int = int4>
   requires(!is_bool<Int>)
class uniform_int_distribution {
 public:
   using result_type = Int;

   class param_type {
    public:
      using distribution_type = uniform_int_distribution;

      constexpr explicit param_type(
         result_type lower = 0, result_type upper = limits<result_type>::max()
      )
          : m_lower(lower), m_upper(upper) {
      }

      constexpr auto
      a() const -> result_type {
         return m_lower;
      }

      constexpr auto
      b() const -> result_type {
         return m_upper;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      result_type m_lower;
      result_type m_upper;
   };

   constexpr uniform_int_distribution() = default;

   constexpr explicit uniform_int_distribution(
      result_type lower, result_type upper = limits<result_type>::max()
   )
       : m_parameters(lower, upper) {
   }

   constexpr explicit uniform_int_distribution(param_type const& parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   a() const -> result_type {
      return m_parameters.a();
   }

   constexpr auto
   b() const -> result_type {
      return m_parameters.b();
   }

   constexpr auto
   min() const -> result_type {
      return a();
   }

   constexpr auto
   max() const -> result_type {
      return b();
   }

   constexpr auto
   param() const -> param_type {
      return m_parameters;
   }

   constexpr void
   param(param_type const& parameters) {
      m_parameters = parameters;
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) const -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameters) const
      -> result_type {
      using unsigned_type = make_unsigned_type<result_type>;
      unsigned_type const lower = unsigned_type(parameters.a());
      unsigned_type const upper = unsigned_type(parameters.b());
      unsigned_type const bound = upper - lower + 1u;
      unsigned_type const offset =
         detail::distribution_random_bounded(generator, bound);
      return detail::distribution_int_from_bits<result_type>(lower + offset);
   }

   template <is_uniform_random_bit_generator Engine, is_simd Simd>
      requires is_same<typename Simd::value_type, typename Engine::result_type>
   constexpr auto
   operator()(independent_simd_engine<Engine, Simd>& engine) const
      -> fixed_size_simd<result_type, Simd::abi_type::lanes> {
      return (*this)(engine, m_parameters);
   }

   template <is_uniform_random_bit_generator Engine, is_simd Simd>
      requires is_same<typename Simd::value_type, typename Engine::result_type>
   constexpr auto
   operator()(
      independent_simd_engine<Engine, Simd>& engine,
      param_type const& parameters
   ) const -> fixed_size_simd<result_type, Simd::abi_type::lanes> {
      fixed_size_simd<result_type, Simd::abi_type::lanes> result;
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         result.set_lane(lane, (*this)(engine.lane(lane), parameters));
      }
      return result;
   }

   friend constexpr auto
   operator==(uniform_int_distribution const&, uniform_int_distribution const&)
      -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
