// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/binomial_distribution.hpp>
#include <cat/detail/cauchy_distribution.hpp>
#include <cat/detail/chi_squared_distribution.hpp>
#include <cat/detail/discrete_distribution.hpp>
#include <cat/detail/exponential_distribution.hpp>
#include <cat/detail/extreme_value_distribution.hpp>
#include <cat/detail/fisher_f_distribution.hpp>
#include <cat/detail/gamma_distribution.hpp>
#include <cat/detail/geometric_distribution.hpp>
#include <cat/detail/lognormal_distribution.hpp>
#include <cat/detail/negative_binomial_distribution.hpp>
#include <cat/detail/normal_distribution.hpp>
#include <cat/detail/piecewise_constant_distribution.hpp>
#include <cat/detail/piecewise_linear_distribution.hpp>
#include <cat/detail/poisson_distribution.hpp>
#include <cat/detail/student_t_distribution.hpp>
#include <cat/detail/uniform_float_distribution.hpp>
#include <cat/detail/uniform_int_distribution.hpp>
#include <cat/detail/weibull_distribution.hpp>

#include <cat/arithmetic>
#include <cat/array>
#include <cat/meta>
#include <cat/random>
#include <cat/simd>

namespace cat {

template <is_simd Simd, typename Distribution>
   requires is_same<
      typename Simd::value_type, typename Distribution::result_type>
class simd_distribution {
 public:
   using result_type = Simd;
   using scalar_distribution_type = Distribution;

   constexpr simd_distribution() = default;

   constexpr explicit simd_distribution(Distribution distribution) {
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         m_distributions[lane] = distribution;
      }
   }

   constexpr void
   reset() {
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         m_distributions[lane].reset();
      }
   }

   template <typename Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      result_type result;
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         if constexpr (requires { generator.lane(lane); }) {
            result.set_lane(lane, m_distributions[lane](generator.lane(lane)));
         } else {
            result.set_lane(lane, m_distributions[lane](generator));
         }
      }
      return result;
   }

 private:
   array<Distribution, Simd::abi_type::lanes> m_distributions;
};

template <is_simd Simd>
using simd_uniform_int_distribution =
   simd_distribution<Simd, uniform_int_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_uniform_float_distribution = simd_distribution<
   Simd, uniform_float_distribution<typename Simd::value_type>>;

template <is_simd Simd, is_floating_point Float = float8>
using simd_binomial_distribution = simd_distribution<
   Simd, binomial_distribution<typename Simd::value_type, Float>>;

template <is_simd Simd, is_floating_point Float = float8>
using simd_negative_binomial_distribution = simd_distribution<
   Simd, negative_binomial_distribution<typename Simd::value_type, Float>>;

template <is_simd Simd, is_floating_point Float = float8>
using simd_geometric_distribution = simd_distribution<
   Simd, geometric_distribution<typename Simd::value_type, Float>>;

template <is_simd Simd, is_floating_point Float = float8>
using simd_poisson_distribution = simd_distribution<
   Simd, poisson_distribution<typename Simd::value_type, Float>>;

template <is_simd Simd>
using simd_exponential_distribution =
   simd_distribution<Simd, exponential_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_gamma_distribution =
   simd_distribution<Simd, gamma_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_weibull_distribution =
   simd_distribution<Simd, weibull_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_extreme_value_distribution = simd_distribution<
   Simd, extreme_value_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_normal_distribution =
   simd_distribution<Simd, normal_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_lognormal_distribution =
   simd_distribution<Simd, lognormal_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_chi_squared_distribution =
   simd_distribution<Simd, chi_squared_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_cauchy_distribution =
   simd_distribution<Simd, cauchy_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_fisher_f_distribution =
   simd_distribution<Simd, fisher_f_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_student_t_distribution =
   simd_distribution<Simd, student_t_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_discrete_distribution =
   simd_distribution<Simd, discrete_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_piecewise_constant_distribution = simd_distribution<
   Simd, piecewise_constant_distribution<typename Simd::value_type>>;

template <is_simd Simd>
using simd_piecewise_linear_distribution = simd_distribution<
   Simd, piecewise_linear_distribution<typename Simd::value_type>>;

}  // namespace cat
