// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

namespace cat {

namespace detail {

// https://prng.di.unimi.it/splitmix64.c
class splitmix64_engine {
 public:
   using result_type = uint8;

   static constexpr random_seed default_seed = 1u;

   constexpr splitmix64_engine() : m_state(result_type(default_seed)) {
   }

   constexpr explicit splitmix64_engine(random_seed value)
       : m_state(result_type(value)) {
   }

   constexpr void
   seed(random_seed value = default_seed) {
      m_state = result_type(value);
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      m_state += increment;
      auto value = m_state;
      value = (value ^ value.shift_right(30u)) * 0xbf58476d'1ce4e5b9ull;
      value = (value ^ value.shift_right(27u)) * 0x94d049bb'133111ebull;
      return result_type(value ^ value.shift_right(31u));
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type {
      return result_type::min();
   }

   [[nodiscard]]
   static constexpr auto
   max() -> result_type {
      return result_type::max();
   }

   // Skip this many outputs.
   constexpr void
   discard(uint8 count) {
      m_state += increment * count;
   }

   // Skip this many outputs backwards.
   constexpr void
   backstep(uint8 count) {
      m_state -= increment * count;
   }

 private:
   static constexpr result_type increment = 0x9e3779b9'7f4a7c15ull;

   wrap_uint8 m_state;
};

}  // namespace detail

using splitmix64_engine = detail::splitmix64_engine;

}  // namespace cat
