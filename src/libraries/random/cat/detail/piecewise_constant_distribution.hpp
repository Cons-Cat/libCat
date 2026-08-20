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

// `cat::piecewise_constant_distribution` samples intervals defined by
// boundaries and constant weights, similar to
// `std::piecewise_constant_distribution`. The constructor normalizes the
// weights. `Float` is the result type and defaults to `float8`. Normalized
// values are read with `.boundaries()` and `.densities()`.
//
// This can be used to sample a continuous density that is constant within
// intervals. For example, it can sample measurements from a histogram.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    piecewise_constant_distribution<float8> piecewise(
//       {0, 1, 2}, {1, 2}
//    );
//    float8 result = piecewise(rng);
//
// Unlike `std::piecewise_constant_distribution`, this implementation supports
// SIMD. Each engine lane produces its own result from the shared intervals:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    piecewise_constant_distribution<float8x2> simd_piecewise(
//       {0, 1, 2}, {1, 2}
//    );
//    float8x2 results = simd_piecewise(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/piecewise_constant_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
class piecewise_constant_distribution {
 public:
   using result_type = Float;
   using scalar_type = detail::random_scalar<Float>;

   class param_type {
    public:
      using distribution_type = piecewise_constant_distribution;

      constexpr param_type() {
         set_default();
      }

      constexpr param_type(
         span<scalar_type const> boundaries, span<scalar_type const> weights
      ) {
         assign(boundaries, weights);
      }

      constexpr param_type(
         initializer_list<scalar_type> boundaries,
         initializer_list<scalar_type> weights
      )
          : param_type(
               span<scalar_type const>{boundaries},
               span<scalar_type const>{weights}
            ) {
      }

      template <typename Function>
      constexpr param_type(
         span<scalar_type const> boundaries, Function function
      ) {
         array<scalar_type, detail::sampling_distribution_capacity> weights{};
         idx const count =
            boundaries.size() == 0u ? 0u : idx(boundaries.size() - 1u);
         idx const usable = count < detail::sampling_distribution_capacity
                               ? count
                               : detail::sampling_distribution_capacity;
         for (idx index = 0u; index < usable; ++index) {
            weights[index] = function(
               (boundaries[index] + boundaries[index + 1u]) / scalar_type(2)
            );
         }
         assign(boundaries, {weights.data(), usable});
      }

      template <typename Function>
      constexpr param_type(
         initializer_list<scalar_type> boundaries, Function function
      )
          : param_type(span<scalar_type const>{boundaries}, function) {
      }

      [[nodiscard]]
      constexpr auto
      boundaries() const -> span<scalar_type const> {
         return {m_boundaries.data(), m_boundary_count};
      }

      [[nodiscard]]
      constexpr auto
      densities() const -> span<scalar_type const> {
         return {m_densities.data(), idx(m_boundary_count - 1u)};
      }

      [[nodiscard]]
      constexpr auto
      operator==(param_type const& other) const -> bool {
         if (m_boundary_count != other.m_boundary_count) {
            return false;
         }
         for (idx index = 0u; index < m_boundary_count; ++index) {
            if (m_boundaries[index] != other.m_boundaries[index]) {
               return false;
            }
         }
         for (idx index = 0u; index + 1u < m_boundary_count; ++index) {
            if (m_densities[index] != other.m_densities[index]) {
               return false;
            }
         }
         return true;
      }

    private:
      friend class piecewise_constant_distribution;

      constexpr void
      set_default() {
         m_boundary_count = 2u;
         m_boundaries[0u] = 0;
         m_boundaries[1u] = 1;
         m_densities[0u] = 1;
         m_cumulative[0u] = 1;
      }

      constexpr void
      assign(
         span<scalar_type const> boundaries, span<scalar_type const> weights
      ) {
         bool valid =
            boundaries.size() >= 2u
            && boundaries.size() <= detail::sampling_distribution_capacity
            && weights.size() == boundaries.size() - 1u;
         if (!valid) {
            assert(valid);
            set_default();
            return;
         }

         scalar_type total = 0;
         for (idx index = 0u; index + 1u < boundaries.size(); ++index) {
            scalar_type const width =
               boundaries[index + 1u] - boundaries[index];
            valid = valid && detail::sampling_finite(boundaries[index])
                    && detail::sampling_finite(boundaries[index + 1u])
                    && detail::sampling_finite(weights[index])
                    && width > scalar_type(0)
                    && weights[index] >= scalar_type(0);
            total += weights[index] * width;
         }
         valid =
            valid && detail::sampling_finite(total) && total > scalar_type(0);
         assert(valid);
         if (!valid) {
            set_default();
            return;
         }

         m_boundary_count = boundaries.size();
         for (idx index = 0u; index < m_boundary_count; ++index) {
            m_boundaries[index] = boundaries[index];
         }
         scalar_type cumulative = 0;
         for (idx index = 0u; index + 1u < m_boundary_count; ++index) {
            m_densities[index] = weights[index] / total;
            cumulative += m_densities[index]
                          * (m_boundaries[index + 1u] - m_boundaries[index]);
            m_cumulative[index] = cumulative;
         }
         m_cumulative[idx(m_boundary_count - 2u)] = 1;
      }

      array<scalar_type, detail::sampling_distribution_capacity> m_boundaries{};
      array<scalar_type, detail::sampling_distribution_capacity> m_densities{};
      array<scalar_type, detail::sampling_distribution_capacity> m_cumulative{};
      idx m_boundary_count = 0u;
   };

   constexpr piecewise_constant_distribution() = default;

   constexpr piecewise_constant_distribution(
      span<scalar_type const> boundaries, span<scalar_type const> weights
   )
       : m_parameters(boundaries, weights) {
   }

   constexpr piecewise_constant_distribution(
      initializer_list<scalar_type> boundaries,
      initializer_list<scalar_type> weights
   )
       : m_parameters(boundaries, weights) {
   }

   template <typename Function>
   constexpr piecewise_constant_distribution(
      span<scalar_type const> boundaries, Function function
   )
       : m_parameters(boundaries, function) {
   }

   template <typename Function>
   constexpr piecewise_constant_distribution(
      initializer_list<scalar_type> boundaries, Function function
   )
       : m_parameters(boundaries, function) {
   }

   constexpr explicit piecewise_constant_distribution(
      param_type const& parameters
   )
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   [[nodiscard]]
   constexpr auto
   min() const -> result_type {
      return m_parameters.m_boundaries[0u];
   }

   [[nodiscard]]
   constexpr auto
   max() const -> result_type {
      return m_parameters.m_boundaries[idx(m_parameters.m_boundary_count - 1u)];
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
   boundaries() const -> span<scalar_type const> {
      return m_parameters.boundaries();
   }

   [[nodiscard]]
   constexpr auto
   densities() const -> span<scalar_type const> {
      return m_parameters.densities();
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
      if constexpr (is_simd<Float>) {
         Float const target = detail::sampling_unit<Float>(generator);
         Float result{};
         typename Float::mask_type selected(false);
         for (idx interval = 0u; interval + 1u < parameters.m_boundary_count;
              ++interval) {
            Float const previous =
               interval == 0u ? 0.f
                              : parameters.m_cumulative[idx(interval - 1u)];
            Float const lower = parameters.m_boundaries[interval];
            Float const upper = parameters.m_boundaries[interval + 1u];
            Float candidate =
               lower + (target - previous) / parameters.m_densities[interval];
            candidate = simd_select(
               candidate >= upper,
               detail::distribution_next_toward(upper, lower), candidate
            );
            candidate = simd_select(candidate < lower, lower, candidate);
            auto const choose =
               !selected && target < Float(parameters.m_cumulative[interval]);
            result = simd_select(choose, candidate, result);
            selected = selected || choose;
         }
         return result;
      } else {
         Float const target = detail::sampling_unit<Float>(generator);
         idx const interval = detail::sampling_interval(
            parameters.m_cumulative, idx(parameters.m_boundary_count - 1u),
            target
         );
         Float const previous =
            interval == 0u ? 0.f : parameters.m_cumulative[idx(interval - 1u)];
         Float const lower = parameters.m_boundaries[interval];
         Float const upper = parameters.m_boundaries[interval + 1u];
         Float result =
            lower + (target - previous) / parameters.m_densities[interval];
         if (result >= upper) {
            result = detail::distribution_next_toward(upper, lower);
         }
         return result < lower ? lower : result;
      }
   }

   [[nodiscard]]
   constexpr auto
   operator==(piecewise_constant_distribution const& other) const -> bool {
      return m_parameters == other.m_parameters;
   }

 private:
   param_type m_parameters{};
};

}  // namespace cat
