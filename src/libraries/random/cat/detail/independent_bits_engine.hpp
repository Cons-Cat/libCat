// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

namespace cat {

template <
   is_uniform_random_bit_generator Engine, idx bits,
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
      using unsigned_type = make_unsigned_type<result_type>;
      if constexpr (bits == limits<unsigned_type>::digits) {
         return result_type::max();
      } else {
         return result_type((unsigned_type(1) << bits) - 1u);
      }
   }

   constexpr auto
   operator()() -> result_type {
      using unsigned_type = make_unsigned_type<result_type>;
      using engine_unsigned = make_unsigned_type<typename Engine::result_type>;
      engine_unsigned const engine_span =
         engine_unsigned(Engine::max()) - engine_unsigned(Engine::min()) + 1u;
      unsigned_type result = 0;
      if (engine_span == 0u && is_unsigned<typename Engine::result_type>) {
         constexpr idx engine_bits = limits<engine_unsigned>::digits;
         idx produced = 0u;
         while (produced < bits) {
            idx const remaining = idx(bits - produced);
            idx const take = remaining < engine_bits ? remaining : engine_bits;
            unsigned_type value =
               unsigned_type(detail::random_engine_word(m_engine));
            if (take < limits<unsigned_type>::digits) {
               value &= (unsigned_type(1) << take.raw) - 1u;
            }
            result |= value << produced.raw;
            produced += take;
         }
      } else {
         for (idx bit = 0u; bit < bits; ++bit) {
            result |= unsigned_type(detail::distribution_random_bit(m_engine))
                      << bit.raw;
         }
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
