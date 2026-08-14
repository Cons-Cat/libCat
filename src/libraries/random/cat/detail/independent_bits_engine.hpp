// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

namespace cat {

template <
   uniform_random_bit_generator Engine, idx bits,
   is_unsigned_integral T = uint4>
   requires(bits > 0u && bits <= limits<T>::digits)
class independent_bits_engine {
 public:
   using result_type = T;

   constexpr independent_bits_engine() = default;

   constexpr explicit independent_bits_engine(Engine engine)
       : m_engine(move(engine)) {
   }

   constexpr explicit independent_bits_engine(random_seed value)
       : m_engine(value) {
   }

   constexpr void
   seed() {
      m_engine.seed();
   }

   constexpr void
   seed(random_seed value) {
      m_engine.seed(value);
   }

   constexpr auto
   base() const -> Engine const& {
      return m_engine;
   }

   static consteval auto
   min() -> result_type {
      return 0u;
   }

   static consteval auto
   max() -> result_type {
      using raw = raw_arithmetic_type<result_type>;
      using unsigned_type = make_unsigned_type<raw>;
      if constexpr (bits == limits<unsigned_type>::digits) {
         return result_type(static_cast<unsigned_type>(-1));
      } else {
         return result_type((unsigned_type(1) << bits.raw) - 1u);
      }
   }

   constexpr auto
   operator()() -> result_type {
      using raw = raw_arithmetic_type<result_type>;
      using unsigned_type = make_unsigned_type<raw>;
      using engine_raw = raw_arithmetic_type<typename Engine::result_type>;
      constexpr idx engine_bits =
         limits<make_unsigned_type<engine_raw>>::digits;
      unsigned_type result = 0;
      idx produced = 0u;
      while (produced < bits) {
         idx const remaining = idx(bits - produced);
         idx const take = remaining < engine_bits ? remaining : engine_bits;
         unsigned_type value =
            static_cast<unsigned_type>(detail::random_engine_word(m_engine));
         if (take < limits<unsigned_type>::digits) {
            value &= (unsigned_type(1) << take.raw) - 1u;
         }
         result |= value << produced.raw;
         produced += take;
      }
      return result_type(result);
   }

   constexpr void
   discard(uint8 count) {
      while (count != 0u) {
         static_cast<void>((*this)());
         --count;
      }
   }

   friend constexpr auto
   operator==(independent_bits_engine const&, independent_bits_engine const&)
      -> bool = default;

 private:
   Engine m_engine{};
};

}  // namespace cat
