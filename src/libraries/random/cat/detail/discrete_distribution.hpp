// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/array>
#include <cat/random>
#include <cat/span>
#include <cat/utility>

#include "./distribution_helpers.hpp"
#include "./sampling_distribution_helpers.hpp"

// `cat::discrete_distribution` selects a zero-based index from a finite set,
// similar to `std::discrete_distribution`. Constructor weights are normalized
// and read with `.probabilities()`. `Int` is the index result type and defaults
// to `int4`.
//
// This can be used to choose among finitely many outcomes with assigned
// weights. For example, it can select a product using market-share weights.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    discrete_distribution<int4> discrete{1, 2, 3};
//    int4 result = discrete(rng);
//
// Unlike `std::discrete_distribution`, this implementation supports SIMD.
// Each engine lane produces its own result using the shared probabilities:
//
//    pcg_dxsm_engine<uint4x4> rng;
//    discrete_distribution<int4x4> simd_discrete{1, 2, 3};
//    int4x4 results = simd_discrete(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/discrete_distribution

namespace cat {

template <typename Int = int4>
   requires(is_integral<Int> || is_simd_integral<Int>)
class discrete_distribution {
 public:
   using result_type = Int;

   class param_type {
    public:
      using distribution_type = discrete_distribution;

      constexpr param_type() {
         set_default();
      }

      constexpr explicit param_type(span<float8 const> weights) {
         assign(weights);
      }

      constexpr param_type(initializer_list<float8> weights)
          : param_type(span<float8 const>{weights}) {
      }

      [[nodiscard]]
      constexpr auto
      probabilities() const -> span<float8 const> {
         return {m_probabilities.data(), m_size};
      }

      [[nodiscard]]
      constexpr auto
      operator==(param_type const& other) const -> bool {
         if (m_size != other.m_size) {
            return false;
         }
         for (idx index = 0u; index < m_size; ++index) {
            if (m_probabilities[index] != other.m_probabilities[index]) {
               return false;
            }
         }
         return true;
      }

    private:
      friend class discrete_distribution;

      constexpr void
      set_default() {
         m_size = 1u;
         m_probabilities[0u] = 1;
         m_cumulative[0u] = 1;
      }

      constexpr void
      assign(span<float8 const> weights) {
         bool valid = weights.size() <= detail::sampling_distribution_capacity;
         idx const size = weights.size();
         if (size == 0u) {
            set_default();
            return;
         }
         if (!valid) {
            assert(valid);
            set_default();
            return;
         }

         float8 total = 0;
         for (idx index = 0u; index < size; ++index) {
            valid = valid && detail::sampling_finite(weights[index])
                    && weights[index] >= 0;
            total += weights[index];
         }
         valid = valid && detail::sampling_finite(total) && total > 0;
         assert(valid);
         if (!valid) {
            set_default();
            return;
         }

         m_size = size;
         float8 cumulative = 0;
         for (idx index = 0u; index < m_size; ++index) {
            m_probabilities[index] = weights[index] / total;
            cumulative += m_probabilities[index];
            m_cumulative[index] = cumulative;
         }
         m_cumulative[idx(m_size - 1u)] = 1;
      }

      array<float8, detail::sampling_distribution_capacity> m_probabilities;
      array<float8, detail::sampling_distribution_capacity> m_cumulative;
      idx m_size = 0u;
   };

   constexpr discrete_distribution() = default;

   constexpr explicit discrete_distribution(span<float8 const> weights)
       : m_parameters(weights) {
   }

   constexpr discrete_distribution(initializer_list<float8> weights)
       : m_parameters(weights) {
   }

   constexpr explicit discrete_distribution(param_type const& parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   [[nodiscard]]
   constexpr auto
   min() const -> result_type {
      return 0;
   }

   [[nodiscard]]
   constexpr auto
   max() const -> result_type {
      return detail::sampling_integer<result_type>(
         idx(m_parameters.m_size - 1u)
      );
   }

   [[nodiscard]]
   constexpr auto
   param() const -> param_type {
      return m_parameters;
   }

   constexpr void
   param(param_type const& parameters) {
      m_parameters = parameters;
   }

   [[nodiscard]]
   constexpr auto
   probabilities() const -> span<float8 const> {
      return m_parameters.probabilities();
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameters)
      -> result_type {
      if constexpr (is_simd<result_type>) {
         using sample_type = detail::distribution_sampling_float<result_type>;
         sample_type const value =
            detail::sampling_unit<sample_type>(generator);
         result_type selected =
            typename result_type::value_type(idx(parameters.m_size - 1u).raw);
         for (idx index = parameters.m_size; index != 0u;) {
            index = idx(index - 1u);
            auto const choose =
               value < detail::distribution_broadcast<sample_type>(
                  parameters.m_cumulative[index]
               );
            selected = simd_select(
               detail::distribution_mask_cast<result_type>(choose),
               result_type(typename result_type::value_type(index.raw)),
               selected
            );
         }
         return selected;
      } else {
         float8 const value = detail::sampling_unit<float8>(generator);
         idx const selected = detail::sampling_interval(
            parameters.m_cumulative, parameters.m_size, value
         );
         return detail::sampling_integer<result_type>(selected);
      }
   }

   [[nodiscard]]
   constexpr auto
   operator==(discrete_distribution const& other) const -> bool {
      return m_parameters == other.m_parameters;
   }

 private:
   param_type m_parameters{};
};

}  // namespace cat
