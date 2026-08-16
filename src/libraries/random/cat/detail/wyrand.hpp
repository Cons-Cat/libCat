// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

// wyrand is Wang Yi's 64-bit non-cryptographic PRNG from the wyhash family.
// It is a small, fast mixer: add a constant to the state, then wymix the
// state with a xored copy of itself.
//
// This file implements `wyrand_engine`, which produces `uint8` and matches the
// `wyrand` routine in the pinned wyhash header.
//
// Reference
//    https://github.com/wangyi-fudan/wyhash
// Pinned `wyrand` / `wymix`
//    https://github.com/wangyi-fudan/wyhash/blob/d2a305811972f391d472cd57c9d542411773ead1/wyhash.h
//
// This code is tested in `tests/src/test_random.cpp` against that pinned
// first output for seed 0.

namespace cat {

class wyrand_engine {
 public:
   using result_type = uint8;

   static constexpr random_seed default_seed = 0u;

   constexpr wyrand_engine() {
      seed();
   }

   constexpr explicit wyrand_engine(random_seed value) {
      seed(value);
   }

   constexpr void
   seed(random_seed value = default_seed) {
      m_state = result_type(value);
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      m_state += increment;
      return wymix(m_state, m_state ^ 0xe7037ed1'a0b428dbull);
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type {
      return 0u;
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

   [[nodiscard]]
   constexpr auto
   operator==(wyrand_engine const&) const -> bool = default;

 private:
   static constexpr result_type increment = 0xa0761d64'78bd642full;

   [[nodiscard]]
   static constexpr auto
   wymix(result_type left, result_type right) -> result_type {
      __uint128_t const product =
         static_cast<__uint128_t>(make_raw_arithmetic(left))
         * static_cast<__uint128_t>(make_raw_arithmetic(right));
      return result_type(uint8(product) ^ uint8(product >> 64u));
   }

   result_type m_state;
};

}  // namespace cat
