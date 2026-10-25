// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

namespace cat {

template <
   is_uniform_random_bit_generator Engine, idx block_size, idx used_block>
   requires(used_block > 0u && used_block <= block_size)
class discard_block_engine {
 public:
   using result_type = Engine::result_type;

   static constexpr idx block_size_value = block_size;
   static constexpr idx used_block_value = used_block;

   constexpr discard_block_engine() = default;

   constexpr explicit discard_block_engine(Engine engine) noexcept
       : m_engine(move(engine)) {
   }

   constexpr explicit discard_block_engine(random_seed value) noexcept
       : m_engine(value) {
   }

   constexpr void
   seed() noexcept {
      m_engine.seed();
      m_used = 0u;
   }

   constexpr void
   seed(random_seed value) noexcept {
      m_engine.seed(value);
      m_used = 0u;
   }

   constexpr auto
   base() const noexcept -> Engine const& {
      return m_engine;
   }

   static consteval auto
   min() noexcept -> result_type {
      return Engine::min();
   }

   static consteval auto
   max() noexcept -> result_type {
      return Engine::max();
   }

   constexpr auto
   operator()() -> result_type {
      if (m_used == used_block) {
         m_engine.discard(uint8(block_size - used_block));
         m_used = 0u;
      }
      ++m_used;
      return m_engine();
   }

   constexpr void
   discard(uint8 count) noexcept {
      while (count != 0u) {
         static_cast<void>((*this)());
         --count;
      }
   }

   friend constexpr auto
   operator==(discard_block_engine const&, discard_block_engine const&) noexcept
      -> bool = default;

 private:
   Engine m_engine{};
   idx m_used = 0u;
};

}  // namespace cat
