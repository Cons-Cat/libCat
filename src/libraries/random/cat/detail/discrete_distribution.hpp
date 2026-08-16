// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/sampling_distribution_helpers.hpp>

#include <cat/arithmetic>
#include <cat/array>
#include <cat/random>
#include <cat/span>
#include <cat/utility>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/discrete_distribution.html
template <is_integral Int = int4>
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
      float8 const value = detail::sampling_unit<float8>(generator);
      idx const selected = detail::sampling_interval(
         parameters.m_cumulative, parameters.m_size, value
      );
      return detail::sampling_integer<result_type>(selected);
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
