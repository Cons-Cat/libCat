// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

namespace cat {

template <uniform_random_bit_generator Engine, idx table_size>
   requires(table_size > 0u)
class shuffle_order_engine {
 public:
   using result_type = Engine::result_type;

   constexpr shuffle_order_engine() {
      initialize();
   }

   constexpr explicit shuffle_order_engine(Engine engine)
       : m_engine(move(engine)) {
      initialize();
   }

   constexpr explicit shuffle_order_engine(random_seed value)
       : m_engine(value) {
      initialize();
   }

   constexpr void
   seed() {
      m_engine.seed();
      initialize();
   }

   constexpr void
   seed(random_seed value) {
      m_engine.seed(value);
      initialize();
   }

   constexpr auto
   base() const -> Engine const& {
      return m_engine;
   }

   static consteval auto
   min() -> result_type {
      return Engine::min();
   }

   static consteval auto
   max() -> result_type {
      return Engine::max();
   }

   constexpr auto
   operator()() -> result_type {
      using raw = raw_arithmetic_type<result_type>;
      using unsigned_type = make_unsigned_type<raw>;
      unsigned __int128 const numerator =
         static_cast<unsigned __int128>(static_cast<unsigned_type>(
            make_raw_arithmetic(m_value) - make_raw_arithmetic(min())
         ))
         * static_cast<unsigned __int128>(make_raw_arithmetic(table_size));
      unsigned __int128 const denominator =
         static_cast<unsigned __int128>(static_cast<unsigned_type>(
            make_raw_arithmetic(max()) - make_raw_arithmetic(min())
         ))
         + 1u;
      idx const index(static_cast<__SIZE_TYPE__>(numerator / denominator));
      result_type result = m_table[index];
      m_table[index] = m_engine();
      m_value = result;
      return result;
   }

   constexpr void
   discard(uint8 count) {
      while (count != 0u) {
         static_cast<void>((*this)());
         --count;
      }
   }

   friend constexpr auto
   operator==(shuffle_order_engine const&, shuffle_order_engine const&)
      -> bool = default;

 private:
   constexpr void
   initialize() {
      for (idx index = 0u; index < table_size; ++index) {
         m_table[index] = m_engine();
      }
      m_value = m_engine();
   }

   Engine m_engine{};
   array<result_type, table_size> m_table{};
   result_type m_value{};
};

}  // namespace cat
