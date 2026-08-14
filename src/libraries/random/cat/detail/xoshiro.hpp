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

   constexpr splitmix64_engine()
       : m_state(static_cast<result_type>(default_seed)) {
   }

   constexpr explicit splitmix64_engine(random_seed value)
       : m_state(static_cast<result_type>(value)) {
   }

   constexpr void
   seed(random_seed value = default_seed) {
      m_state = static_cast<result_type>(value);
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      m_state = m_state + 0x9e3779b9'7f4a7c15ull;
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

   constexpr void
   discard(uint8 count) {
      for (uint8 iteration = 0u; iteration < count; ++iteration) {
         static_cast<void>(operator()());
      }
   }

 private:
   wrap_uint8 m_state;
};

template <typename Derived, typename Word, idx state_size>
class xoshiro_engine_base;

template <xoshiro_scrambler scrambler>
class xoroshiro64_engine;

template <xoshiro_scrambler scrambler>
class xoshiro128_engine;

template <xoshiro_scrambler scrambler>
class xoroshiro128_engine;

template <xoshiro_scrambler scrambler>
class xoshiro256_engine;

template <xoshiro_scrambler scrambler>
class xoshiro512_engine;

template <xoshiro_scrambler scrambler>
class xoroshiro1024_engine;

}  // namespace detail

using xoroshiro64_star =
   detail::xoroshiro64_engine<detail::xoshiro_scrambler::star>;
using xoroshiro64_starstar =
   detail::xoroshiro64_engine<detail::xoshiro_scrambler::starstar>;

using xoshiro128_plus =
   detail::xoshiro128_engine<detail::xoshiro_scrambler::plus>;
using xoshiro128_starstar =
   detail::xoshiro128_engine<detail::xoshiro_scrambler::starstar>;

using xoroshiro128_plus =
   detail::xoroshiro128_engine<detail::xoshiro_scrambler::plus>;
using xoroshiro128_plusplus =
   detail::xoroshiro128_engine<detail::xoshiro_scrambler::plusplus>;
using xoroshiro128_starstar =
   detail::xoroshiro128_engine<detail::xoshiro_scrambler::starstar>;

using xoshiro256_plus =
   detail::xoshiro256_engine<detail::xoshiro_scrambler::plus>;
using xoshiro256_starstar =
   detail::xoshiro256_engine<detail::xoshiro_scrambler::starstar>;

using xoshiro512_plus =
   detail::xoshiro512_engine<detail::xoshiro_scrambler::plus>;
using xoshiro512_plusplus =
   detail::xoshiro512_engine<detail::xoshiro_scrambler::plusplus>;
using xoshiro512_starstar =
   detail::xoshiro512_engine<detail::xoshiro_scrambler::starstar>;

using xoroshiro1024_star =
   detail::xoroshiro1024_engine<detail::xoshiro_scrambler::star>;
using xoroshiro1024_plusplus =
   detail::xoroshiro1024_engine<detail::xoshiro_scrambler::plusplus>;
using xoroshiro1024_starstar =
   detail::xoroshiro1024_engine<detail::xoshiro_scrambler::starstar>;

using splitmix64 = detail::splitmix64_engine;

}  // namespace cat

namespace cat {

// https://prng.di.unimi.it/xoshiro128plusplus.c
template <is_simd Simd, detail::xoshiro_scrambler scrambler>
   requires is_same<typename Simd::value_type, uint4>
class simd_xoshiro128_engine {
 public:
   using result_type = Simd;

   constexpr simd_xoshiro128_engine() {
      seed(1u);
   }

   constexpr explicit simd_xoshiro128_engine(random_seed value) {
      seed(value);
   }

   constexpr void
   seed(random_seed value = 1u) {
      auto const seed_value = static_cast<uint4>(value);
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         splitmix64 mixer(
            uint8(seed_value) + uint8(lane) * 0x9e3779b9'7f4a7c15ull
         );
         for (idx word = 0u; word < 4u; ++word) {
            m_state[word].set_lane(lane, uint4(mixer()));
         }
      }
   }

   constexpr auto
   operator()() -> result_type {
      result_type result;
      if constexpr (scrambler == detail::xoshiro_scrambler::plus) {
         result = m_state[0u] + m_state[3u];
      } else if constexpr (scrambler == detail::xoshiro_scrambler::plusplus) {
         result = simd_rotate_left(m_state[0u] + m_state[3u], 7u) + m_state[0u];
      } else {
         result = simd_rotate_left(m_state[1u] * uint4(5u), 7u) * uint4(9u);
      }

      result_type const temporary = m_state[1u] << 9u;
      m_state[2u] ^= m_state[0u];
      m_state[3u] ^= m_state[1u];
      m_state[1u] ^= m_state[2u];
      m_state[0u] ^= m_state[3u];
      m_state[2u] ^= temporary;
      m_state[3u] = simd_rotate_left(m_state[3u], 11u);
      return result;
   }

   constexpr void
   discard(uint8 count) {
      while (count != 0u) {
         static_cast<void>((*this)());
         --count;
      }
   }

 private:
   array<result_type, 4u> m_state;
};

template <typename Abi>
using simd_xoshiro128_plus =
   simd_xoshiro128_engine<simd<uint4, Abi>, detail::xoshiro_scrambler::plus>;

template <typename Abi>
using simd_xoshiro128_starstar = simd_xoshiro128_engine<
   simd<uint4, Abi>, detail::xoshiro_scrambler::starstar>;

// https://prng.di.unimi.it/xoshiro256plusplus.c
template <is_simd Simd, detail::xoshiro_scrambler scrambler>
   requires is_same<typename Simd::value_type, uint8>
class simd_xoshiro256_engine {
 public:
   using result_type = Simd;

   constexpr simd_xoshiro256_engine() {
      seed(1u);
   }

   constexpr explicit simd_xoshiro256_engine(random_seed value) {
      seed(value);
   }

   constexpr void
   seed(random_seed value = 1u) {
      auto const seed_value = static_cast<uint8>(value);
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         splitmix64 mixer(seed_value + uint8(lane) * 0x9e3779b9'7f4a7c15ull);
         for (idx word = 0u; word < 4u; ++word) {
            m_state[word].set_lane(lane, mixer());
         }
      }
   }

   constexpr auto
   operator()() -> result_type {
      result_type result;
      if constexpr (scrambler == detail::xoshiro_scrambler::plus) {
         result = m_state[0u] + m_state[3u];
      } else if constexpr (scrambler == detail::xoshiro_scrambler::plusplus) {
         result =
            simd_rotate_left(m_state[0u] + m_state[3u], 23u) + m_state[0u];
      } else {
         result = simd_rotate_left(m_state[1u] * uint8(5u), 7u) * uint8(9u);
      }

      result_type const temporary = m_state[1u] << 17u;
      m_state[2u] ^= m_state[0u];
      m_state[3u] ^= m_state[1u];
      m_state[1u] ^= m_state[2u];
      m_state[0u] ^= m_state[3u];
      m_state[2u] ^= temporary;
      m_state[3u] = simd_rotate_left(m_state[3u], 45u);
      return result;
   }

   constexpr void
   discard(uint8 count) {
      while (count != 0u) {
         static_cast<void>((*this)());
         --count;
      }
   }

 private:
   array<result_type, 4u> m_state;
};

template <typename Abi>
using simd_xoshiro256_plus =
   simd_xoshiro256_engine<simd<uint8, Abi>, detail::xoshiro_scrambler::plus>;

template <typename Abi>
using simd_xoshiro256_starstar = simd_xoshiro256_engine<
   simd<uint8, Abi>, detail::xoshiro_scrambler::starstar>;

}  // namespace cat

namespace cat::detail {

template <typename Derived, typename Word, idx state_size>
class xoshiro_engine_base {
 public:
   using result_type = Word;

   static constexpr random_seed default_seed = 1u;

 protected:
   using state_word =
      conditional<is_same<result_type, uint4>, wrap_uint4, wrap_uint8>;
   using state_type = array<state_word, state_size>;

 public:
   constexpr xoshiro_engine_base() {
      seed();
   }

   constexpr explicit xoshiro_engine_base(random_seed value) {
      seed(value);
   }

   template <typename... Values>
      requires(sizeof...(Values) == state_size && sizeof...(Values) > 1u)
   constexpr explicit xoshiro_engine_base(Values... values)
       : m_state(state_word(values)...) {
      prevent_zero_state();
   }

   constexpr void
   seed(random_seed value = default_seed) {
      splitmix64_engine mixer{static_cast<uint8>(value)};
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         m_state[state_index] = state_word(mixer());
      }
      prevent_zero_state();
   }

 protected:
   template <idx polynomial_size>
   constexpr void
   apply_jump(array<state_word, polynomial_size> const& polynomial) {
      state_type state;
      state.fill(0u);
      for (idx polynomial_index = 0u; polynomial_index < polynomial_size;
           ++polynomial_index) {
         for (uword bit = 0u; bit < sizeof(state_word) * 8u; ++bit) {
            if (
               (polynomial[polynomial_index] & state_word{1u}.shift_left(bit))
               != 0u
            ) {
               for (idx word = 0u; word < state_size; ++word) {
                  state[word] ^= m_state[word];
               }
            }
            static_cast<Derived&>(*this).next_state();
         }
      }
      m_state = state;
   }

   constexpr void
   prevent_zero_state() {
      state_word combined = 0u;
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         combined |= m_state[state_index];
      }
      if (combined == 0u) {
         m_state[0u] = 1u;
      }
   }

   constexpr void
   discard(uint8 count) {
      for (uint8 iteration = 0u; iteration < count; ++iteration) {
         static_cast<Derived&>(*this).next_state();
      }
   }

   state_type m_state;
};

// https://prng.di.unimi.it/xoroshiro64star.c
// https://prng.di.unimi.it/xoroshiro64starstar.c
template <xoshiro_scrambler scrambler>
class xoroshiro64_engine
    : public xoshiro_engine_base<xoroshiro64_engine<scrambler>, uint4, 2u> {
 private:
   using base = xoshiro_engine_base<xoroshiro64_engine<scrambler>, uint4, 2u>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      state_word result;
      if constexpr (scrambler == xoshiro_scrambler::star) {
         result = m_state[0u] * 0x9e3779bbul;
      } else {
         result = (m_state[0u] * 0x9e3779bbul).rotate_left(5u) * 5u;
      }
      next_state();
      return result_type(result);
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

   constexpr void
   jump() {
      this->apply_jump(
         array<state_word, 2u>{
            0x77fcd1a0ul,
            0x4cbf99bdul,
         }
      );
   }

   constexpr void
   long_jump() {
      this->apply_jump(
         array<state_word, 2u>{
            0x3f1f8b95ul,
            0xb4e7e463ul,
         }
      );
   }

 private:
   constexpr void
   next_state() {
      auto const state_0 = m_state[0u];
      auto state_1 = m_state[1u];
      state_1 ^= state_0;
      m_state[0u] = state_0.rotate_left(26u) ^ state_1 ^ state_1.shift_left(9u);
      m_state[1u] = state_1.rotate_left(13u);
   }
};

// https://prng.di.unimi.it/xoshiro128plus.c
// https://prng.di.unimi.it/xoshiro128plusplus.c
// https://prng.di.unimi.it/xoshiro128starstar.c
template <xoshiro_scrambler scrambler>
class xoshiro128_engine
    : public xoshiro_engine_base<xoshiro128_engine<scrambler>, uint4, 4u> {
 private:
   using base = xoshiro_engine_base<xoshiro128_engine<scrambler>, uint4, 4u>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      state_word result;
      if constexpr (scrambler == xoshiro_scrambler::plus) {
         result = m_state[0u] + m_state[3u];
      } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         result = (m_state[0u] + m_state[3u]).rotate_left(7u) + m_state[0u];
      } else {
         result = (m_state[1u] * 5u).rotate_left(7u) * 9u;
      }
      next_state();
      return result_type(result);
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

   constexpr void
   jump() {
      this->apply_jump(
         array<state_word, 4u>{
            0x8764000bul,
            0xf542d2d3ul,
            0x6fa035c3ul,
            0x77f2db5bul,
         }
      );
   }

   constexpr void
   long_jump() {
      this->apply_jump(
         array<state_word, 4u>{
            0xb523952eul,
            0x0b6f099ful,
            0xccf5a0eful,
            0x1c580662ul,
         }
      );
   }

 private:
   constexpr void
   next_state() {
      auto const temporary = m_state[1u].shift_left(9u);
      m_state[2u] ^= m_state[0u];
      m_state[3u] ^= m_state[1u];
      m_state[1u] ^= m_state[2u];
      m_state[0u] ^= m_state[3u];
      m_state[2u] ^= temporary;
      m_state[3u] = m_state[3u].rotate_left(11u);
   }
};

// https://prng.di.unimi.it/xoroshiro128plus.c
// https://prng.di.unimi.it/xoroshiro128plusplus.c
// https://prng.di.unimi.it/xoroshiro128starstar.c
template <xoshiro_scrambler scrambler>
class xoroshiro128_engine
    : public xoshiro_engine_base<xoroshiro128_engine<scrambler>, uint8, 2u> {
 private:
   using base = xoshiro_engine_base<xoroshiro128_engine<scrambler>, uint8, 2u>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const state_0 = m_state[0u];
      auto const state_1 = m_state[1u];
      state_word result;
      if constexpr (scrambler == xoshiro_scrambler::plus) {
         result = state_0 + state_1;
      } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         result = (state_0 + state_1).rotate_left(17u) + state_0;
      } else {
         result = (state_0 * 5u).rotate_left(7u) * 9u;
      }
      next_state();
      return result_type(result);
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

   constexpr void
   jump() {
      if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         this->apply_jump(
            array<state_word, 2u>{
               0x2bd7a6a6'e99c2ddcull,
               0x0992ccaf'6a6fca05ull,
            }
         );
      } else {
         this->apply_jump(
            array<state_word, 2u>{
               0xdf900294'd8f554a5ull,
               0x170865df'4b3201fcull,
            }
         );
      }
   }

   constexpr void
   long_jump() {
      if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         this->apply_jump(
            array<state_word, 2u>{
               0x360fd5f2'cf8d5d99ull,
               0x9c6e6877'736c46e3ull,
            }
         );
      } else {
         this->apply_jump(
            array<state_word, 2u>{
               0xd2a98b26'625eee7bull,
               0xdddf9b10'90aa7ac1ull,
            }
         );
      }
   }

 private:
   constexpr void
   next_state() {
      auto const state_0 = m_state[0u];
      auto state_1 = m_state[1u];
      state_1 ^= state_0;
      if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         m_state[0u] =
            state_0.rotate_left(49u) ^ state_1 ^ state_1.shift_left(21u);
         m_state[1u] = state_1.rotate_left(28u);
      } else {
         m_state[0u] =
            state_0.rotate_left(24u) ^ state_1 ^ state_1.shift_left(16u);
         m_state[1u] = state_1.rotate_left(37u);
      }
   }
};

// https://prng.di.unimi.it/xoshiro256plus.c
// https://prng.di.unimi.it/xoshiro256plusplus.c
// https://prng.di.unimi.it/xoshiro256starstar.c
template <xoshiro_scrambler scrambler>
class xoshiro256_engine
    : public xoshiro_engine_base<xoshiro256_engine<scrambler>, uint8, 4u> {
 private:
   using base = xoshiro_engine_base<xoshiro256_engine<scrambler>, uint8, 4u>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      state_word result;
      if constexpr (scrambler == xoshiro_scrambler::plus) {
         result = m_state[0u] + m_state[3u];
      } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         result = (m_state[0u] + m_state[3u]).rotate_left(23u) + m_state[0u];
      } else {
         result = (m_state[1u] * 5u).rotate_left(7u) * 9u;
      }
      next_state();
      return result_type(result);
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

   constexpr void
   jump() {
      this->apply_jump(
         array<state_word, 4u>{
            0x180ec6d3'3cfd0abaull,
            0xd5a61266'f0c9392cull,
            0xa9582618'e03fc9aaull,
            0x39abdc45'29b1661cull,
         }
      );
   }

   constexpr void
   long_jump() {
      this->apply_jump(
         array<state_word, 4u>{
            0x76e15d3e'fefdcbbfull,
            0xc5004e44'1c522fb3ull,
            0x77710069'854ee241ull,
            0x39109bb0'2acbe635ull,
         }
      );
   }

 private:
   constexpr void
   next_state() {
      auto const temporary = m_state[1u].shift_left(17u);
      m_state[2u] ^= m_state[0u];
      m_state[3u] ^= m_state[1u];
      m_state[1u] ^= m_state[2u];
      m_state[0u] ^= m_state[3u];
      m_state[2u] ^= temporary;
      m_state[3u] = m_state[3u].rotate_left(45u);
   }
};

// https://prng.di.unimi.it/xoshiro512plus.c
// https://prng.di.unimi.it/xoshiro512plusplus.c
// https://prng.di.unimi.it/xoshiro512starstar.c
template <xoshiro_scrambler scrambler>
class xoshiro512_engine
    : public xoshiro_engine_base<xoshiro512_engine<scrambler>, uint8, 8u> {
 private:
   using base = xoshiro_engine_base<xoshiro512_engine<scrambler>, uint8, 8u>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      state_word result;
      if constexpr (scrambler == xoshiro_scrambler::plus) {
         result = m_state[0u] + m_state[2u];
      } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         result = (m_state[0u] + m_state[2u]).rotate_left(17u) + m_state[2u];
      } else {
         result = (m_state[1u] * 5u).rotate_left(7u) * 9u;
      }
      next_state();
      return result_type(result);
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

   constexpr void
   jump() {
      this->apply_jump(
         array<state_word, 8u>{
            0x33ed89b6'e7a353f9ull,
            0x760083d7'955323beull,
            0x2837f2fb'b5f22faeull,
            0x4b8c5674'd309511cull,
            0xb11ac47a'7ba28c25ull,
            0xf1be7667'092bcc1cull,
            0x53851efd'b6df0aafull,
            0x1ebbc8b2'3eaf25dbull,
         }
      );
   }

   constexpr void
   long_jump() {
      this->apply_jump(
         array<state_word, 8u>{
            0x11467fef'8f921d28ull,
            0xa2a819f2'e79c8ea8ull,
            0xa8299fc2'84b3959aull,
            0xb4d34734'0ca63ee1ull,
            0x1cb0940b'edbff6ceull,
            0xd956c5c4'fa1f8e17ull,
            0x915e38fd'4eda93bcull,
            0x5b3ccdfa'5d7daca5ull,
         }
      );
   }

 private:
   constexpr void
   next_state() {
      auto const temporary = m_state[1u].shift_left(11u);
      m_state[2u] ^= m_state[0u];
      m_state[5u] ^= m_state[1u];
      m_state[1u] ^= m_state[2u];
      m_state[7u] ^= m_state[3u];
      m_state[3u] ^= m_state[4u];
      m_state[4u] ^= m_state[5u];
      m_state[0u] ^= m_state[6u];
      m_state[6u] ^= m_state[7u];
      m_state[6u] ^= temporary;
      m_state[7u] = m_state[7u].rotate_left(21u);
   }
};

// https://prng.di.unimi.it/xoroshiro1024star.c
// https://prng.di.unimi.it/xoroshiro1024plusplus.c
// https://prng.di.unimi.it/xoroshiro1024starstar.c
template <xoshiro_scrambler scrambler>
class xoroshiro1024_engine {
 public:
   using result_type = uint8;

   static constexpr random_seed default_seed = 1u;

 private:
   using state_word = wrap_uint8;
   static constexpr idx state_size = 16u;

 public:
   constexpr xoroshiro1024_engine() {
      seed();
   }

   constexpr explicit xoroshiro1024_engine(random_seed value) {
      seed(value);
   }

   template <typename... Values>
      requires(sizeof...(Values) == state_size)
   constexpr explicit xoroshiro1024_engine(Values... values)
       : m_state(state_word(values)...) {
      prevent_zero_state();
   }

   constexpr void
   seed(random_seed value = default_seed) {
      splitmix64_engine mixer(static_cast<result_type>(value));
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         m_state[state_index] = state_word(mixer());
      }
      m_position = 0u;
      prevent_zero_state();
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      idx const previous = m_position;
      ++m_position;
      if (m_position == state_size) {
         m_position = 0u;
      }

      auto const state_0 = m_state[m_position];
      auto state_15 = m_state[previous];
      state_word result;
      if constexpr (scrambler == xoshiro_scrambler::star) {
         result = state_0 * 0x9e3779b9'7f4a7c13ull;
      } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         result = (state_0 + state_15).rotate_left(23u) + state_15;
      } else {
         result = (state_0 * 5u).rotate_left(7u) * 9u;
      }

      state_15 ^= state_0;
      m_state[previous] =
         state_0.rotate_left(25u) ^ state_15 ^ state_15.shift_left(27u);
      m_state[m_position] = state_15.rotate_left(36u);
      return result_type(result);
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

   constexpr void
   discard(uint8 count) {
      for (uint8 iteration = 0u; iteration < count; ++iteration) {
         static_cast<void>(operator()());
      }
   }

   constexpr void
   jump() {
      apply_jump(
         array<state_word, 16u>{
            0x931197d8'e3177f17ull,
            0xb59422e0'b9138c5full,
            0xf06a6afb'49d668bbull,
            0xacb8a641'2c8a1401ull,
            0x12304ec8'5f0b3468ull,
            0xb7dfe707'9209891eull,
            0x405b7eec'77d9eb14ull,
            0x34ead682'80c44e4aull,
            0xe0e4ba3e'0ac9e366ull,
            0x8f46eda8'348905b7ull,
            0x328bf4db'ad90d6ffull,
            0xc8fd6fb3'1c9effc3ull,
            0xe899d452'd4b67652ull,
            0x45f38728'6ade3205ull,
            0x03864f45'4a8920bdull,
            0xa68fa287'25b1b384ull,
         }
      );
   }

   constexpr void
   long_jump() {
      apply_jump(
         array<state_word, 16u>{
            0x73741563'60bbf00full,
            0x4630c2ef'a3b3c1f6ull,
            0x6654183a'892786b1ull,
            0x94f7bfcb'fb0f1661ull,
            0x27d8243d'3d13eb2dull,
            0x9701730f'3dfb300full,
            0x2f293baa'e6f604adull,
            0xa661831c'b60cd8b6ull,
            0x68280c77'd9fe008cull,
            0x50554160'f5ba9459ull,
            0x2fc20b17'ec7b2a9aull,
            0x49189bbd'c8ec9f8full,
            0x92a65bca'41852cc1ull,
            0xf46820dd'0509c12aull,
            0x52b00c35'fbf92185ull,
            0x1e5b3b7f'589e03c1ull,
         }
      );
   }

 private:
   [[nodiscard]]
   constexpr auto
   physical_index(idx logical) const -> idx {
      idx result = logical + m_position;
      if (result >= state_size) {
         result = idx(result - state_size);
      }
      return result;
   }

   constexpr void
   prevent_zero_state() {
      state_word combined = 0u;
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         combined |= m_state[state_index];
      }
      if (combined == 0u) {
         m_state[0u] = 1u;
      }
   }

   constexpr void
   apply_jump(array<state_word, 16u> const& polynomial) {
      array<state_word, 16u> state;
      state.fill(0u);
      for (idx polynomial_index = 0u; polynomial_index < state_size;
           ++polynomial_index) {
         for (uword bit = 0u; bit < 64u; ++bit) {
            if (
               (polynomial[polynomial_index] & state_word{1u}.shift_left(bit))
               != 0u
            ) {
               for (idx word = 0u; word < state_size; ++word) {
                  state[word] ^= m_state[physical_index(word)];
               }
            }
            static_cast<void>(operator()());
         }
      }
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         m_state[physical_index(state_index)] = state[state_index];
      }
   }

   idx m_position = 0u;
   array<state_word, 16u> m_state;
};

}  // namespace cat::detail
