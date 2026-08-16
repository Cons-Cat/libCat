// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

namespace cat {

template <is_uniform_random_bit_generator Engine, is_simd Simd>
   requires is_same<typename Simd::value_type, typename Engine::result_type>
class independent_simd_engine {
 public:
   using result_type = Simd;
   using scalar_engine_type = Engine;
   using scalar_result_type = Engine::result_type;

   constexpr independent_simd_engine() {
      seed(0u);
   }

   constexpr explicit independent_simd_engine(random_seed value) {
      seed(value);
   }

   constexpr void
   seed(random_seed value) {
      auto lane_seed = scalar_result_type(value);
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         lane_seed =
            detail::random_mix_seed(lane_seed + scalar_result_type(lane));
         m_engines[lane].seed(lane_seed);
      }
   }

   constexpr auto
   operator()() -> result_type {
      result_type result;
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         result.set_lane(lane, m_engines[lane]());
      }
      return result;
   }

   constexpr auto
   lane(idx index) -> Engine& {
      return m_engines[index];
   }

   constexpr auto
   lane(idx index) const -> Engine const& {
      return m_engines[index];
   }

   constexpr void
   discard(uint8 count) {
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         m_engines[lane].discard(count);
      }
   }

 private:
   array<Engine, Simd::abi_type::lanes> m_engines{};
};

}  // namespace cat
