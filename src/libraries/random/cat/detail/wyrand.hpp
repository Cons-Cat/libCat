// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

namespace cat {

// Reference:
// https://github.com/wangyi-fudan/wyhash/blob/d2a305811972f391d472cd57c9d542411773ead1/wyhash.h
class wyrand {
 public:
   using result_type = uint8;

   static constexpr random_seed default_seed = 0u;

   constexpr wyrand() {
      seed();
   }

   constexpr explicit wyrand(random_seed value) {
      seed(value);
   }

   constexpr void
   seed(random_seed value = default_seed) {
      m_state = static_cast<result_type>(value);
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      m_state += 0xa0761d64'78bd642full;
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

   constexpr void
   discard(uint8 count) {
      for (uint8 iteration = 0u; iteration < count; ++iteration) {
         static_cast<void>(operator()());
      }
   }

   [[nodiscard]]
   constexpr auto
   operator==(wyrand const&) const -> bool = default;

 private:
   [[nodiscard]]
   static constexpr auto
   wymix(result_type left, result_type right) -> result_type {
      unsigned __int128 const product =
         static_cast<unsigned __int128>(left.raw) * right.raw;
      return result_type(
         static_cast<result_type::raw_type>(product)
         ^ static_cast<result_type::raw_type>(product >> 64u)
      );
   }

   result_type m_state;
};

}  // namespace cat
