// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Xoshiro ("Xor/Shift/Rotate") is a fast non-cryptographic PRNG engine. It is a
// popular high quality algorithm family that performs faster than other
// algorithms in its same class.
//    https://prng.di.unimi.it/
//
// This file implements:
//    `xoshiro128+`/`xoshiro128++`/`xoshiro128**`
//    `xoshiro256+`/`xoshiro256++`/`xoshiro256**`
//    `xoshiro512+`/`xoshiro512++`/`xoshiro512**`
//    `xoroshiro64*`/`xoroshiro64**`
//    `xoroshiro128+`/`xoroshiro128++`/`xoroshiro128**`
//    `xoroshiro1024++`/`xoroshiro1024*`/`xoroshiro1024**`
// Along with SIMD variants of the same scrambles.
//
// `xoshiro_engine<T>` is the `**` scramble. `xoshiro_pp_engine<T>` is `++`.
// The same split applies to xoroshiro, xoshiro512, and xoroshiro1024.
//
// Not that Xoshiro has slightly inferior statistical properties to libCat's
// default, PCG:
//    https://www.pcg-random.org/posts/on-vignas-pcg-critique.html
//    https://www.pcg-random.org/posts/xoshiro-repeat-flaws.html
// However, Xoshiro might be faster.
//
// This implementation borrows a technique from:
//    https://github.com/nessan/xoshiro
//    https://nessan.github.io/xoshiro/md_docs_2pages_2jump-technique.html
//
// This code is well tested in `tests/src/test_xoshiro.cpp`, but links to
// reference source code are provided in this file for validation.

#include <cat/random>

#include "./xoshiro_jump.hpp"

namespace cat::detail {

template <
   typename Derived, is_simd Simd, idx state_size, bool narrow_seed = false>
class simd_xoshiro_engine_base {
 public:
   using result_type = Simd;

   static constexpr random_seed default_seed = 1u;

 protected:
   using state_word = result_type::value_type;
   using state_type = array<result_type, state_size>;

 public:
   constexpr simd_xoshiro_engine_base() {
      seed();
   }

   constexpr explicit simd_xoshiro_engine_base(random_seed value) {
      seed(value);
   }

   constexpr void
   seed(random_seed value = default_seed) {
      auto const seed_value = uint8(value);
      if constexpr (is_same<state_word, uint8>) {
         result_type mixer_state =
            seed_value
            + iota<result_type>(0u) * state_word(0x9e3779b9'7f4a7c15ull);
         for (idx word = 0u; word < state_size; ++word) {
            mixer_state += state_word(0x9e3779b9'7f4a7c15ull);
            auto mixed = mixer_state;
            mixed =
               (mixed ^ (mixed >> 30u)) * state_word(0xbf58476d'1ce4e5b9ull);
            mixed =
               (mixed ^ (mixed >> 27u)) * state_word(0x94d049bb'133111ebull);
            m_state[word] = mixed ^ (mixed >> 31u);
         }
      } else {
         auto const lane_seed =
            narrow_seed ? uint8(uint4(value)) : uint8(value);
         for (idx lane = 0u; lane < result_type::abi_type::lanes; ++lane) {
            splitmix64_engine mixer(
               lane_seed + uint8(lane) * 0x9e3779b9'7f4a7c15ull
            );
            for (idx word = 0u; word < state_size; ++word) {
               m_state[word].set_lane(lane, state_word(mixer()));
            }
         }
      }
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type {
      return state_word::min();
   }

   [[nodiscard]]
   static constexpr auto
   max() -> result_type {
      return state_word::max();
   }

   // Skip this many outputs.
   constexpr void
   discard(uint8 count) {
      for (uint8 iteration = 0u; iteration < count; ++iteration) {
         static_cast<Derived&>(*this).next_state();
      }
   }

 protected:
   template <idx polynomial_size>
   constexpr void
   apply_jump(array<state_word, polynomial_size> const& polynomial) {
      state_type state;
      state.fill(result_type{});
      for (idx polynomial_index = 0u; polynomial_index < polynomial_size;
           ++polynomial_index) {
         for (uword bit = 0u; bit < sizeof(state_word) * 8u; ++bit) {
            if (
               (polynomial[polynomial_index] & (state_word(1u) << bit)) != 0u
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

   state_type m_state;
};

template <typename Word>
struct xoshiro_word_operations {
   template <typename Value>
   static constexpr auto
   constant(Value value) -> Word {
      return Word(value);
   }

   static constexpr auto
   rotate_left(Word value, unsigned count) -> Word {
      return value.rotate_left(count);
   }

   static constexpr auto
   shift_left(Word value, unsigned count) -> Word {
      return value.shift_left(count);
   }
};

template <is_simd Word>
struct xoshiro_word_operations<Word> {
   using value_type = Word::value_type;

   template <typename Value>
   static constexpr auto
   constant(Value value) -> value_type {
      return value_type(value);
   }

   static constexpr auto
   rotate_left(Word value, unsigned count) -> Word {
      return simd_rotate_left(value, count);
   }

   static constexpr auto
   shift_left(Word value, unsigned count) -> Word {
      return value << count;
   }
};

template <
   xoshiro_scrambler scrambler, unsigned plusplus_rotation, typename Word>
constexpr auto
xoshiro_scramble_0_3(array<Word, 4u> const& state) -> Word {
   using operations = xoshiro_word_operations<Word>;
   if constexpr (scrambler == xoshiro_scrambler::plus) {
      return state[0u] + state[3u];
   } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
      return operations::rotate_left(state[0u] + state[3u], plusplus_rotation)
             + state[0u];
   } else {
      return operations::rotate_left(state[1u] * operations::constant(5u), 7u)
             * operations::constant(9u);
   }
}

template <typename Word>
constexpr void
xoshiro_four_word_transition(
   array<Word, 4u>& state, unsigned shift, unsigned rotation
) {
   using operations = xoshiro_word_operations<Word>;
   auto const temporary = operations::shift_left(state[1u], shift);
   state[2u] ^= state[0u];
   state[3u] ^= state[1u];
   state[1u] ^= state[2u];
   state[0u] ^= state[3u];
   state[2u] ^= temporary;
   state[3u] = operations::rotate_left(state[3u], rotation);
}

template <xoshiro_scrambler scrambler>
struct xoshiro128_family {
   template <typename Word>
   static constexpr auto
   scramble(array<Word, 4u> const& state) -> Word {
      return xoshiro_scramble_0_3<scrambler, 7u>(state);
   }

   template <typename Word>
   static constexpr void
   transition(array<Word, 4u>& state) {
      xoshiro_four_word_transition(state, 9u, 11u);
   }

   template <typename Word>
   static constexpr auto
   jump() -> array<Word, 4u> {
      return {0x8764000bu, 0xf542d2d3u, 0x6fa035c3u, 0x77f2db5bu};
   }

   template <typename Word>
   static constexpr auto
   long_jump() -> array<Word, 4u> {
      return {0xb523952eu, 0x0b6f099fu, 0xccf5a0efu, 0x1c580662u};
   }
};

template <xoshiro_scrambler scrambler>
struct xoshiro256_family {
   template <typename Word>
   static constexpr auto
   scramble(array<Word, 4u> const& state) -> Word {
      return xoshiro_scramble_0_3<scrambler, 23u>(state);
   }

   template <typename Word>
   static constexpr void
   transition(array<Word, 4u>& state) {
      xoshiro_four_word_transition(state, 17u, 45u);
   }

   template <typename Word>
   static constexpr auto
   jump() -> array<Word, 4u> {
      return {
         0x180ec6d3'3cfd0abaull,
         0xd5a61266'f0c9392cull,
         0xa9582618'e03fc9aaull,
         0x39abdc45'29b1661cull,
      };
   }

   template <typename Word>
   static constexpr auto
   long_jump() -> array<Word, 4u> {
      return {
         0x76e15d3e'fefdcbbfull,
         0xc5004e44'1c522fb3ull,
         0x77710069'854ee241ull,
         0x39109bb0'2acbe635ull,
      };
   }
};

// https://prng.di.unimi.it/xoshiro128plusplus.c
template <is_simd Simd, xoshiro_scrambler scrambler>
   requires is_same<typename Simd::value_type, uint4>
class simd_xoshiro128_engine
    : public simd_xoshiro_engine_base<
         simd_xoshiro128_engine<Simd, scrambler>, Simd, 4u, true> {
 private:
   using base = simd_xoshiro_engine_base<
      simd_xoshiro128_engine<Simd, scrambler>, Simd, 4u, true>;
   using family = xoshiro128_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::max;
   using base::min;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
      next_state();
      return result;
   }

   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
   }
};

// https://prng.di.unimi.it/xoshiro256plusplus.c
template <is_simd Simd, xoshiro_scrambler scrambler>
   requires is_same<typename Simd::value_type, uint8>
class simd_xoshiro256_engine
    : public simd_xoshiro_engine_base<
         simd_xoshiro256_engine<Simd, scrambler>, Simd, 4u> {
 private:
   using base = simd_xoshiro_engine_base<
      simd_xoshiro256_engine<Simd, scrambler>, Simd, 4u>;
   using family = xoshiro256_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::max;
   using base::min;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
      next_state();
      return result;
   }

   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
   }
};

template <xoshiro_scrambler scrambler>
struct xoroshiro64_family {
   template <typename Word>
   static constexpr auto
   scramble(array<Word, 2u> const& state) -> Word {
      using operations = xoshiro_word_operations<Word>;
      auto const multiplier = operations::constant(0x9e3779bbu);
      if constexpr (scrambler == xoshiro_scrambler::star) {
         return state[0u] * multiplier;
      } else {
         return operations::rotate_left(state[0u] * multiplier, 5u)
                * operations::constant(5u);
      }
   }

   template <typename Word>
   static constexpr void
   transition(array<Word, 2u>& state) {
      using operations = xoshiro_word_operations<Word>;
      auto const state_0 = state[0u];
      auto state_1 = state[1u];
      state_1 ^= state_0;
      state[0u] = operations::rotate_left(state_0, 26u) ^ state_1
                  ^ operations::shift_left(state_1, 9u);
      state[1u] = operations::rotate_left(state_1, 13u);
   }

   template <typename Word>
   static constexpr auto
   jump() -> array<Word, 2u> {
      return {0x77fcd1a0u, 0x4cbf99bdu};
   }

   template <typename Word>
   static constexpr auto
   long_jump() -> array<Word, 2u> {
      return {0x3f1f8b95u, 0xb4e7e463u};
   }
};

template <xoshiro_scrambler scrambler>
struct xoroshiro128_family {
   template <typename Word>
   static constexpr auto
   scramble(array<Word, 2u> const& state) -> Word {
      using operations = xoshiro_word_operations<Word>;
      auto const state_0 = state[0u];
      auto const state_1 = state[1u];
      if constexpr (scrambler == xoshiro_scrambler::plus) {
         return state_0 + state_1;
      } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         return operations::rotate_left(state_0 + state_1, 17u) + state_0;
      } else {
         return operations::rotate_left(state_0 * operations::constant(5u), 7u)
                * operations::constant(9u);
      }
   }

   template <typename Word>
   static constexpr void
   transition(array<Word, 2u>& state) {
      using operations = xoshiro_word_operations<Word>;
      auto const state_0 = state[0u];
      auto state_1 = state[1u];
      state_1 ^= state_0;
      if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         state[0u] = operations::rotate_left(state_0, 49u) ^ state_1
                     ^ operations::shift_left(state_1, 21u);
         state[1u] = operations::rotate_left(state_1, 28u);
      } else {
         state[0u] = operations::rotate_left(state_0, 24u) ^ state_1
                     ^ operations::shift_left(state_1, 16u);
         state[1u] = operations::rotate_left(state_1, 37u);
      }
   }

   template <typename Word>
   static constexpr auto
   jump() -> array<Word, 2u> {
      if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         return {0x2bd7a6a6'e99c2ddcull, 0x0992ccaf'6a6fca05ull};
      } else {
         return {0xdf900294'd8f554a5ull, 0x170865df'4b3201fcull};
      }
   }

   template <typename Word>
   static constexpr auto
   long_jump() -> array<Word, 2u> {
      if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         return {0x360fd5f2'cf8d5d99ull, 0x9c6e6877'736c46e3ull};
      } else {
         return {0xd2a98b26'625eee7bull, 0xdddf9b10'90aa7ac1ull};
      }
   }
};

template <xoshiro_scrambler scrambler>
struct xoshiro512_family {
   template <typename Word>
   static constexpr auto
   scramble(array<Word, 8u> const& state) -> Word {
      using operations = xoshiro_word_operations<Word>;
      if constexpr (scrambler == xoshiro_scrambler::plus) {
         return state[0u] + state[2u];
      } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         return operations::rotate_left(state[0u] + state[2u], 17u) + state[2u];
      } else {
         return operations::rotate_left(
                   state[1u] * operations::constant(5u), 7u
                )
                * operations::constant(9u);
      }
   }

   template <typename Word>
   static constexpr void
   transition(array<Word, 8u>& state) {
      using operations = xoshiro_word_operations<Word>;
      auto const temporary = operations::shift_left(state[1u], 11u);
      state[2u] ^= state[0u];
      state[5u] ^= state[1u];
      state[1u] ^= state[2u];
      state[7u] ^= state[3u];
      state[3u] ^= state[4u];
      state[4u] ^= state[5u];
      state[0u] ^= state[6u];
      state[6u] ^= state[7u];
      state[6u] ^= temporary;
      state[7u] = operations::rotate_left(state[7u], 21u);
   }

   template <typename Word>
   static constexpr auto
   jump() -> array<Word, 8u> {
      return {
         0x33ed89b6'e7a353f9ull, 0x760083d7'955323beull, 0x2837f2fb'b5f22faeull,
         0x4b8c5674'd309511cull, 0xb11ac47a'7ba28c25ull, 0xf1be7667'092bcc1cull,
         0x53851efd'b6df0aafull, 0x1ebbc8b2'3eaf25dbull,
      };
   }

   template <typename Word>
   static constexpr auto
   long_jump() -> array<Word, 8u> {
      return {
         0x11467fef'8f921d28ull, 0xa2a819f2'e79c8ea8ull, 0xa8299fc2'84b3959aull,
         0xb4d34734'0ca63ee1ull, 0x1cb0940b'edbff6ceull, 0xd956c5c4'fa1f8e17ull,
         0x915e38fd'4eda93bcull, 0x5b3ccdfa'5d7daca5ull,
      };
   }
};

template <xoshiro_scrambler scrambler, typename Word>
constexpr auto
xoroshiro1024_scramble(Word state_0, Word state_15) -> Word {
   using operations = xoshiro_word_operations<Word>;
   if constexpr (scrambler == xoshiro_scrambler::star) {
      return state_0 * operations::constant(0x9e3779b9'7f4a7c13ull);
   } else if constexpr (scrambler == xoshiro_scrambler::plusplus) {
      return operations::rotate_left(state_0 + state_15, 23u) + state_15;
   } else {
      return operations::rotate_left(state_0 * operations::constant(5u), 7u)
             * operations::constant(9u);
   }
}

template <typename Word>
constexpr void
xoroshiro1024_transition(
   Word state_0, Word state_15, Word& next_0, Word& next_15
) {
   using operations = xoshiro_word_operations<Word>;
   state_15 ^= state_0;
   next_15 = operations::rotate_left(state_0, 25u) ^ state_15
             ^ operations::shift_left(state_15, 27u);
   next_0 = operations::rotate_left(state_15, 36u);
}

template <typename Word>
constexpr auto
xoroshiro1024_jump() -> array<Word, 16u> {
   return {
      0x931197d8'e3177f17ull, 0xb59422e0'b9138c5full, 0xf06a6afb'49d668bbull,
      0xacb8a641'2c8a1401ull, 0x12304ec8'5f0b3468ull, 0xb7dfe707'9209891eull,
      0x405b7eec'77d9eb14ull, 0x34ead682'80c44e4aull, 0xe0e4ba3e'0ac9e366ull,
      0x8f46eda8'348905b7ull, 0x328bf4db'ad90d6ffull, 0xc8fd6fb3'1c9effc3ull,
      0xe899d452'd4b67652ull, 0x45f38728'6ade3205ull, 0x03864f45'4a8920bdull,
      0xa68fa287'25b1b384ull,
   };
}

template <typename Word>
constexpr auto
xoroshiro1024_long_jump() -> array<Word, 16u> {
   return {
      0x73741563'60bbf00full, 0x4630c2ef'a3b3c1f6ull, 0x6654183a'892786b1ull,
      0x94f7bfcb'fb0f1661ull, 0x27d8243d'3d13eb2dull, 0x9701730f'3dfb300full,
      0x2f293baa'e6f604adull, 0xa661831c'b60cd8b6ull, 0x68280c77'd9fe008cull,
      0x50554160'f5ba9459ull, 0x2fc20b17'ec7b2a9aull, 0x49189bbd'c8ec9f8full,
      0x92a65bca'41852cc1ull, 0xf46820dd'0509c12aull, 0x52b00c35'fbf92185ull,
      0x1e5b3b7f'589e03c1ull,
   };
}

template <is_simd Simd, xoshiro_scrambler scrambler>
class simd_xoroshiro_engine
    : public simd_xoshiro_engine_base<
         simd_xoroshiro_engine<Simd, scrambler>, Simd, 2u> {
 private:
   using base = simd_xoshiro_engine_base<
      simd_xoroshiro_engine<Simd, scrambler>, Simd, 2u>;
   using family = conditional<
      is_same<typename Simd::value_type, uint4>, xoroshiro64_family<scrambler>,
      xoroshiro128_family<scrambler>>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::max;
   using base::min;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
      next_state();
      return result;
   }

   // Published stream-split jump.
   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
   }
};

template <is_simd Simd, xoshiro_scrambler scrambler>
   requires is_same<typename Simd::value_type, uint8>
class simd_xoshiro512_engine
    : public simd_xoshiro_engine_base<
         simd_xoshiro512_engine<Simd, scrambler>, Simd, 8u> {
 private:
   using base = simd_xoshiro_engine_base<
      simd_xoshiro512_engine<Simd, scrambler>, Simd, 8u>;
   using family = xoshiro512_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::max;
   using base::min;
   using base::seed;

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
      next_state();
      return result;
   }

   // Published stream-split jump.
   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
   }
};

template <is_simd Simd, xoshiro_scrambler scrambler>
   requires is_same<typename Simd::value_type, uint8>
class simd_xoroshiro1024_engine
    : public simd_xoshiro_engine_base<
         simd_xoroshiro1024_engine<Simd, scrambler>, Simd, 16u> {
 private:
   using base = simd_xoshiro_engine_base<
      simd_xoroshiro1024_engine<Simd, scrambler>, Simd, 16u>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

   static constexpr idx state_size = 16u;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::max;
   using base::min;

   constexpr void
   seed(random_seed value = default_seed) {
      base::seed(value);
      m_position = 0u;
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      idx const previous = m_position;
      idx next = previous + 1u;
      if (next == state_size) {
         next = 0u;
      }
      auto const state_0 = m_state[next];
      auto const state_15 = m_state[previous];
      auto const result = xoroshiro1024_scramble<scrambler>(state_0, state_15);
      next_state();
      return result;
   }

   // Published stream-split jump.
   constexpr void
   jump() {
      apply_jump(xoroshiro1024_jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      apply_jump(xoroshiro1024_long_jump<state_word>());
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
   apply_jump(array<state_word, 16u> const& polynomial) {
      array<result_type, 16u> state;
      state.fill(result_type{});
      for (idx polynomial_index = 0u; polynomial_index < state_size;
           ++polynomial_index) {
         for (uword bit = 0u; bit < 64u; ++bit) {
            if (
               (polynomial[polynomial_index] & (state_word(1u) << bit)) != 0u
            ) {
               for (idx word = 0u; word < state_size; ++word) {
                  state[word] ^= m_state[physical_index(word)];
               }
            }
            next_state();
         }
      }
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         m_state[physical_index(state_index)] = state[state_index];
      }
   }

   constexpr void
   next_state() {
      idx const previous = m_position;
      ++m_position;
      if (m_position == state_size) {
         m_position = 0u;
      }
      auto const state_0 = m_state[m_position];
      auto const state_15 = m_state[previous];
      xoroshiro1024_transition(
         state_0, state_15, m_state[m_position], m_state[previous]
      );
   }

   idx m_position = 0u;
};

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
      splitmix64_engine mixer{uint8(value)};
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         m_state[state_index] = state_word(mixer());
      }
      prevent_zero_state();
   }

   // Apply a jump polynomial.
   constexpr void
   jump(array<state_word, state_size> const& polynomial) {
      apply_jump(polynomial);
   }

   // Skip 2^log2_distance outputs.
   constexpr void
   jump_log2(uword log2_distance) {
      apply_jump(jump_polynomial(log2_distance, true));
   }

   // Polynomial that skips `distance` outputs, or 2^distance if
   // `distance_is_log2`.
   [[nodiscard]]
   static constexpr auto
   jump_polynomial(uword distance, bool distance_is_log2)
      -> array<state_word, state_size> {
      using raw = raw_arithmetic_type<state_word>;
      array<raw, state_size> characteristic;
      auto const words = Derived::characteristic_coefficients();
      for (idx word = 0u; word < state_size; ++word) {
         characteristic[word] = raw(words[word]);
      }
      auto const reduced =
         xoshiro_jump_polynomial(characteristic, distance, distance_is_log2);
      array<state_word, state_size> result;
      for (idx word = 0u; word < state_size; ++word) {
         result[word] = state_word(reduced[word]);
      }
      return result;
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

   // Skip this many outputs.
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
   using family = xoroshiro64_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::jump;
   using base::jump_log2;
   using base::jump_polynomial;
   using base::seed;

   static constexpr auto
   characteristic_coefficients() -> array<state_word, 2u> {
      return {0x6e2286c1ul, 0x053be9daul};
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
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

   // Published stream-split jump.
   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
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
   using family = xoshiro128_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::jump;
   using base::jump_log2;
   using base::jump_polynomial;
   using base::seed;

   static constexpr auto
   characteristic_coefficients() -> array<state_word, 4u> {
      return {0xde18fc01ul, 0x1b489db6ul, 0x006254b1ul, 0x00fc65a2ul};
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
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

   // Published stream-split jump.
   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
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
   using family = xoroshiro128_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::jump;
   using base::jump_log2;
   using base::jump_polynomial;
   using base::seed;

   static constexpr auto
   characteristic_coefficients() -> array<state_word, 2u> {
      if constexpr (scrambler == xoshiro_scrambler::plusplus) {
         return {0x8dae7077'9760b081ull, 0x0031bcf2'f855d6e5ull};
      } else {
         return {0x095b8f76'579aa001ull, 0x0008828e'513b43d5ull};
      }
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
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

   // Published stream-split jump.
   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
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
   using family = xoshiro256_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::jump;
   using base::jump_log2;
   using base::jump_polynomial;
   using base::seed;

   static constexpr auto
   characteristic_coefficients() -> array<state_word, 4u> {
      return {
         0x9d116f2b'b0f0f001ull,
         0x0280002b'cefd1a5eull,
         0x04b4edcf'26259f85ull,
         0x0003c03c'3f3ecb19ull,
      };
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
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

   // Published stream-split jump.
   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
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
   using family = xoshiro512_family<scrambler>;
   using state_word = base::state_word;
   using base::m_state;
   friend base;

 public:
   using result_type = base::result_type;
   using base::base;
   using base::default_seed;
   using base::discard;
   using base::jump;
   using base::jump_log2;
   using base::jump_polynomial;
   using base::seed;

   static constexpr auto
   characteristic_coefficients() -> array<state_word, 8u> {
      return {
         0xcf3cff0c'00000001ull, 0x7fdc78d8'86f00c63ull, 0xf05e63fc'a6d7b781ull,
         0x7a67058e'7bbab6f0ull, 0xf11eef83'2e32518full, 0x51ba7c47'edc758adull,
         0x8f2d2726'8ce4b20bull, 0x00005000'55d8b77full,
      };
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const result = family::scramble(m_state);
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

   // Published stream-split jump.
   constexpr void
   jump() {
      this->apply_jump(family::template jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      this->apply_jump(family::template long_jump<state_word>());
   }

 private:
   constexpr void
   next_state() {
      family::transition(m_state);
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
      splitmix64_engine mixer{result_type(value)};
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         m_state[state_index] = state_word(mixer());
      }
      m_position = 0u;
      prevent_zero_state();
   }

   static constexpr auto
   characteristic_coefficients() -> array<state_word, 16u> {
      return {
         0x5cfeb8cc'48ddb211ull, 0xb73e379d'035a06ddull, 0x17d5100a'20a0350eull,
         0x7550223f'68f98cacull, 0x29d373b5'c5ed3459ull, 0x3689b412'ef70de48ull,
         0xa1d3b6ee'079a7cc6ull, 0x9bf0b669'abd100f8ull, 0x955c84e1'05f60997ull,
         0x6ca140c6'1889cdddull, 0xabaf68c5'fc3a0e4aull, 0xa4613452'6b83adc5ull,
         0x0710704d'05683d63ull, 0x580d080b'44b606a2ull, 0x008040a0'580158a1ull,
         0x00000000'00800081ull,
      };
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      idx const previous = m_position;
      idx next = previous + 1u;
      if (next == state_size) {
         next = 0u;
      }
      auto const state_0 = m_state[next];
      auto const state_15 = m_state[previous];
      auto const result = xoroshiro1024_scramble<scrambler>(state_0, state_15);
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

   // Skip this many outputs.
   constexpr void
   discard(uint8 count) {
      for (uint8 iteration = 0u; iteration < count; ++iteration) {
         next_state();
      }
   }

   // Apply a jump polynomial.
   constexpr void
   jump(array<state_word, state_size> const& polynomial) {
      apply_jump(polynomial);
   }

   // Skip 2^log2_distance outputs.
   constexpr void
   jump_log2(uword log2_distance) {
      apply_jump(jump_polynomial(log2_distance, true));
   }

   // Polynomial that skips `distance` outputs, or 2^distance if
   // `distance_is_log2`.
   [[nodiscard]]
   static constexpr auto
   jump_polynomial(uword distance, bool distance_is_log2)
      -> array<state_word, state_size> {
      using raw = raw_arithmetic_type<state_word>;
      array<raw, state_size> characteristic;
      auto const words = characteristic_coefficients();
      for (idx word = 0u; word < state_size; ++word) {
         characteristic[word] = raw(words[word]);
      }
      auto const reduced =
         xoshiro_jump_polynomial(characteristic, distance, distance_is_log2);
      array<state_word, state_size> result;
      for (idx word = 0u; word < state_size; ++word) {
         result[word] = state_word(reduced[word]);
      }
      return result;
   }

   // Published stream-split jump.
   constexpr void
   jump() {
      apply_jump(xoroshiro1024_jump<state_word>());
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump() {
      apply_jump(xoroshiro1024_long_jump<state_word>());
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
            next_state();
         }
      }
      for (idx state_index = 0u; state_index < state_size; ++state_index) {
         m_state[physical_index(state_index)] = state[state_index];
      }
   }

   constexpr void
   next_state() {
      idx const previous = m_position;
      ++m_position;
      if (m_position == state_size) {
         m_position = 0u;
      }
      auto const state_0 = m_state[m_position];
      auto const state_15 = m_state[previous];
      xoroshiro1024_transition(
         state_0, state_15, m_state[m_position], m_state[previous]
      );
   }

   idx m_position = 0u;
   array<state_word, 16u> m_state;
};

// https://prng.di.unimi.it/
// Default engines are `**`. `*_pp_engine` is `++`. `+` is an explicit scramble.
template <typename T>
consteval auto
xoshiro_word_bytes() -> idx {
   if constexpr (is_simd<T>) {
      return sizeof(typename T::value_type);
   }
   return sizeof(raw_arithmetic_type<T>);
}

template <typename T, xoshiro_scrambler scrambler>
struct xoshiro_for;

template <typename T, xoshiro_scrambler scrambler>
   requires(!is_simd<T> && xoshiro_word_bytes<T>() == 4u)
struct xoshiro_for<T, scrambler> {
   using type = xoshiro128_engine<scrambler>;
};

template <typename T, xoshiro_scrambler scrambler>
   requires(!is_simd<T> && xoshiro_word_bytes<T>() == 8u)
struct xoshiro_for<T, scrambler> {
   using type = xoshiro256_engine<scrambler>;
};

template <is_simd Simd, xoshiro_scrambler scrambler>
   requires(sizeof(typename Simd::value_type) == 4u)
struct xoshiro_for<Simd, scrambler> {
   using unsigned_lane = uint4;
   using unsigned_simd = simd<
      unsigned_lane,
      typename Simd::abi_type::template make_abi_type<unsigned_lane>>;
   using type = simd_xoshiro128_engine<unsigned_simd, scrambler>;
};

template <is_simd Simd, xoshiro_scrambler scrambler>
   requires(sizeof(typename Simd::value_type) == 8u)
struct xoshiro_for<Simd, scrambler> {
   using unsigned_lane = uint8;
   using unsigned_simd = simd<
      unsigned_lane,
      typename Simd::abi_type::template make_abi_type<unsigned_lane>>;
   using type = simd_xoshiro256_engine<unsigned_simd, scrambler>;
};

template <typename T, xoshiro_scrambler scrambler>
struct xoroshiro_for;

template <typename T, xoshiro_scrambler scrambler>
   requires(!is_simd<T> && xoshiro_word_bytes<T>() == 4u)
struct xoroshiro_for<T, scrambler> {
   static constexpr auto mapped = (scrambler == xoshiro_scrambler::plus
                                   || scrambler == xoshiro_scrambler::star)
                                     ? xoshiro_scrambler::star
                                     : xoshiro_scrambler::starstar;
   using type = xoroshiro64_engine<mapped>;
};

template <typename T, xoshiro_scrambler scrambler>
   requires(!is_simd<T> && xoshiro_word_bytes<T>() == 8u)
struct xoroshiro_for<T, scrambler> {
   static constexpr auto mapped = scrambler == xoshiro_scrambler::star
                                     ? xoshiro_scrambler::starstar
                                     : scrambler;
   using type = xoroshiro128_engine<mapped>;
};

template <is_simd Simd, xoshiro_scrambler scrambler>
   requires(sizeof(typename Simd::value_type) == 4u)
struct xoroshiro_for<Simd, scrambler> {
   using unsigned_lane = uint4;
   using unsigned_simd = simd<
      unsigned_lane,
      typename Simd::abi_type::template make_abi_type<unsigned_lane>>;
   static constexpr auto mapped = (scrambler == xoshiro_scrambler::plus
                                   || scrambler == xoshiro_scrambler::star)
                                     ? xoshiro_scrambler::star
                                     : xoshiro_scrambler::starstar;
   using type = simd_xoroshiro_engine<unsigned_simd, mapped>;
};

template <is_simd Simd, xoshiro_scrambler scrambler>
   requires(sizeof(typename Simd::value_type) == 8u)
struct xoroshiro_for<Simd, scrambler> {
   using unsigned_lane = uint8;
   using unsigned_simd = simd<
      unsigned_lane,
      typename Simd::abi_type::template make_abi_type<unsigned_lane>>;
   static constexpr auto mapped = scrambler == xoshiro_scrambler::star
                                     ? xoshiro_scrambler::starstar
                                     : scrambler;
   using type = simd_xoroshiro_engine<unsigned_simd, mapped>;
};

template <typename T, xoshiro_scrambler scrambler>
struct xoshiro512_for;

template <typename T, xoshiro_scrambler scrambler>
   requires(!is_simd<T>)
struct xoshiro512_for<T, scrambler> {
   using type = xoshiro512_engine<scrambler>;
};

template <is_simd Simd, xoshiro_scrambler scrambler>
struct xoshiro512_for<Simd, scrambler> {
   using unsigned_simd =
      simd<uint8, typename Simd::abi_type::template make_abi_type<uint8>>;
   using type = simd_xoshiro512_engine<unsigned_simd, scrambler>;
};

template <typename T, xoshiro_scrambler scrambler>
struct xoroshiro1024_for;

template <typename T, xoshiro_scrambler scrambler>
   requires(!is_simd<T>)
struct xoroshiro1024_for<T, scrambler> {
   static constexpr auto mapped = scrambler == xoshiro_scrambler::plus
                                     ? xoshiro_scrambler::plusplus
                                     : scrambler;
   using type = xoroshiro1024_engine<mapped>;
};

template <is_simd Simd, xoshiro_scrambler scrambler>
struct xoroshiro1024_for<Simd, scrambler> {
   using unsigned_simd =
      simd<uint8, typename Simd::abi_type::template make_abi_type<uint8>>;
   static constexpr auto mapped = scrambler == xoshiro_scrambler::plus
                                     ? xoshiro_scrambler::plusplus
                                     : scrambler;
   using type = simd_xoroshiro1024_engine<unsigned_simd, mapped>;
};

template <typename T, xoshiro_scrambler scrambler>
using xoshiro_selected = xoshiro_for<T, scrambler>::type;

template <typename T, xoshiro_scrambler scrambler>
using xoroshiro_selected = xoroshiro_for<T, scrambler>::type;

template <typename T, xoshiro_scrambler scrambler>
using xoshiro512_selected = xoshiro512_for<T, scrambler>::type;

template <typename T, xoshiro_scrambler scrambler>
using xoroshiro1024_selected = xoroshiro1024_for<T, scrambler>::type;

template <typename Engine>
class owned_engine {
 public:
   using result_type = Engine::result_type;

   constexpr owned_engine() = default;

   constexpr explicit owned_engine(random_seed value) : m_engine(value) {
   }

   template <typename... Values>
      requires(sizeof...(Values) > 1u)
              && requires(Values... values) { Engine(values...); }
   constexpr explicit owned_engine(Values... values) : m_engine(values...) {
   }

   constexpr void
   seed(random_seed value = 1u) {
      m_engine.seed(value);
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      return m_engine();
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type
      requires requires { Engine::min(); }
   {
      return Engine::min();
   }

   [[nodiscard]]
   static constexpr auto
   max() -> result_type
      requires requires { Engine::max(); }
   {
      return Engine::max();
   }

   // Skip this many outputs.
   constexpr void
   discard(uint8 count) {
      m_engine.discard(count);
   }

   // Skip this many outputs backwards.
   template <typename Distance>
   constexpr void
   backstep(Distance distance)
      requires requires(Engine& engine) { engine.backstep(distance); }
   {
      m_engine.backstep(distance);
   }

   // Published stream-split jump.
   constexpr void
   jump()
      requires requires(Engine& engine) { engine.jump(); }
   {
      m_engine.jump();
   }

   // Larger published stream-split jump.
   constexpr void
   long_jump()
      requires requires(Engine& engine) { engine.long_jump(); }
   {
      m_engine.long_jump();
   }

   // Apply a jump polynomial.
   template <typename Polynomial>
   constexpr void
   jump(Polynomial const& polynomial)
      requires requires(Engine& engine) { engine.jump(polynomial); }
   {
      m_engine.jump(polynomial);
   }

   // Skip 2^log2_distance outputs.
   constexpr void
   jump_log2(uword log2_distance)
      requires requires(Engine& engine) { engine.jump_log2(log2_distance); }
   {
      m_engine.jump_log2(log2_distance);
   }

   // Polynomial that skips `distance` outputs, or 2^distance if
   // `distance_is_log2`.
   [[nodiscard]]
   static constexpr auto
   jump_polynomial(uword distance, bool distance_is_log2)
      requires requires { Engine::jump_polynomial(distance, distance_is_log2); }
   {
      return Engine::jump_polynomial(distance, distance_is_log2);
   }

 private:
   Engine m_engine;
};

}  // namespace cat::detail

namespace cat {

template <
   typename T,
   detail::xoshiro_scrambler scrambler = detail::xoshiro_scrambler::starstar>
class xoshiro_engine
    : public detail::owned_engine<detail::xoshiro_selected<T, scrambler>> {
   using base = detail::owned_engine<detail::xoshiro_selected<T, scrambler>>;

 public:
   using base::base;
};

template <
   typename T,
   detail::xoshiro_scrambler scrambler = detail::xoshiro_scrambler::starstar>
class xoroshiro_engine
    : public detail::owned_engine<detail::xoroshiro_selected<T, scrambler>> {
   using base = detail::owned_engine<detail::xoroshiro_selected<T, scrambler>>;

 public:
   using base::base;
};

template <
   typename T,
   detail::xoshiro_scrambler scrambler = detail::xoshiro_scrambler::starstar>
   requires(detail::xoshiro_word_bytes<T>() == 8u)
class xoshiro512_engine
    : public detail::owned_engine<detail::xoshiro512_selected<T, scrambler>> {
   using base = detail::owned_engine<detail::xoshiro512_selected<T, scrambler>>;

 public:
   using base::base;
};

template <
   typename T,
   detail::xoshiro_scrambler scrambler = detail::xoshiro_scrambler::starstar>
   requires(detail::xoshiro_word_bytes<T>() == 8u)
class xoroshiro1024_engine : public detail::owned_engine<
                                detail::xoroshiro1024_selected<T, scrambler>> {
   using base =
      detail::owned_engine<detail::xoroshiro1024_selected<T, scrambler>>;

 public:
   using base::base;
};

template <typename T>
class xoshiro_pp_engine
    : public detail::owned_engine<
         detail::xoshiro_selected<T, detail::xoshiro_scrambler::plusplus>> {
   using base = detail::owned_engine<
      detail::xoshiro_selected<T, detail::xoshiro_scrambler::plusplus>>;

 public:
   using base::base;
};

template <typename T>
class xoroshiro_pp_engine
    : public detail::owned_engine<
         detail::xoroshiro_selected<T, detail::xoshiro_scrambler::plusplus>> {
   using base = detail::owned_engine<
      detail::xoroshiro_selected<T, detail::xoshiro_scrambler::plusplus>>;

 public:
   using base::base;
};

template <typename T>
   requires(detail::xoshiro_word_bytes<T>() == 8u)
class xoshiro512_pp_engine
    : public detail::owned_engine<
         detail::xoshiro512_selected<T, detail::xoshiro_scrambler::plusplus>> {
   using base = detail::owned_engine<
      detail::xoshiro512_selected<T, detail::xoshiro_scrambler::plusplus>>;

 public:
   using base::base;
};

template <typename T>
   requires(detail::xoshiro_word_bytes<T>() == 8u)
class xoroshiro1024_pp_engine
    : public detail::owned_engine<detail::xoroshiro1024_selected<
         T, detail::xoshiro_scrambler::plusplus>> {
   using base = detail::owned_engine<
      detail::xoroshiro1024_selected<T, detail::xoshiro_scrambler::plusplus>>;

 public:
   using base::base;
};

}  // namespace cat
