// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

// Reference repository
// https://github.com/imneme/pcg-c
// Pinned constants, seeding, streams, and permutations
// https://github.com/imneme/pcg-c/blob/83252d9c23df9c82ecb42210afed61a7b42402d7/include/pcg_variants.h

namespace cat {

enum class pcg_stream : uint1::raw_type {
   setseq,
   oneseq,
   unique,
   mcg,
};

enum class pcg_permutation : uint1::raw_type {
   xorshift_high_random_rotate,
   xorshift_high_random_shift,
   xorshift_low_random_rotate,
   random_xorshift_multiply_xorshift,
};

namespace detail {

template <typename State>
struct pcg_constants;

template <>
struct pcg_constants<uint8> {
   static constexpr uint8 multiplier = 6'364'136'223'846'793'005ull;
   static constexpr uint8 increment = 1'442'695'040'888'963'407ull;

   static constexpr uint8 oneseq_state = 0x4d595df4'd0f33173ull;
   static constexpr uint8 mcg_state = 0xcafef00d'd15ea5e5ull;
   static constexpr uint8 setseq_state = 0x853c49e6'748fea9bull;
   static constexpr uint8 setseq_increment = 0xda3e39cb'94b95bdbull;
};

template <>
struct pcg_constants<unsigned __int128> {
   static constexpr unsigned __int128 multiplier =
      (static_cast<unsigned __int128>(2'549'297'995'355'413'924ull) << 64u)
      | 4'865'540'595'714'422'341ull;
   static constexpr unsigned __int128 increment =
      (static_cast<unsigned __int128>(6'364'136'223'846'793'005ull) << 64u)
      | 1'442'695'040'888'963'407ull;

   static constexpr unsigned __int128 oneseq_state =
      (static_cast<unsigned __int128>(0xb8dc10e1'58a92392ull) << 64u)
      | 0x98046df0'07ec0a53ull;
   static constexpr unsigned __int128 mcg_state = 0xcafef00d'd15ea5e5ull;
   static constexpr unsigned __int128 setseq_state =
      (static_cast<unsigned __int128>(0x979c9a98'd8462005ull) << 64u)
      | 0x7d3e9cb6'cfe0549bull;
   static constexpr unsigned __int128 setseq_increment =
      (static_cast<unsigned __int128>(1u) << 64u) | 0xda3e39cb'94b95bdbull;
};

template <typename State>
[[nodiscard]]
constexpr auto
pcg_initial_state(pcg_stream stream) -> State {
   if (stream == pcg_stream::mcg) {
      return pcg_constants<State>::mcg_state;
   }
   if (stream == pcg_stream::setseq) {
      return pcg_constants<State>::setseq_state;
   }
   return pcg_constants<State>::oneseq_state;
}

template <typename State>
[[nodiscard]]
constexpr auto
pcg_initial_increment(pcg_stream stream) -> State {
   if (stream == pcg_stream::setseq) {
      return pcg_constants<State>::setseq_increment;
   }
   if (stream == pcg_stream::oneseq) {
      return pcg_constants<State>::increment;
   }
   return 0u;
}

template <typename Result, typename State, pcg_permutation permutation>
[[nodiscard]]
constexpr auto
pcg_output(State state) -> Result {
   auto const raw_state = make_raw_arithmetic(state);

   if constexpr (permutation == pcg_permutation::xorshift_high_random_rotate) {
      static_assert(sizeof(State) == 8u && sizeof(Result) == 4u);
      uint4 const word =
         uint4::raw_type(((raw_state >> 18u) ^ raw_state) >> 27u);
      return rotate_right(word, uword(raw_state >> 59u));
   }

   if constexpr (permutation == pcg_permutation::xorshift_high_random_shift) {
      static_assert(sizeof(State) == 8u && sizeof(Result) == 4u);
      return uint4::raw_type(
         ((raw_state >> 22u) ^ raw_state) >> ((raw_state >> 61u) + 22u)
      );
   }

   if constexpr (permutation == pcg_permutation::xorshift_low_random_rotate) {
      static_assert(sizeof(State) == 16u && sizeof(Result) == 8u);
      uint8 const word = uint8::raw_type((raw_state >> 64u) ^ raw_state);
      return rotate_right(word, uword(raw_state >> 122u));
   }

   if constexpr (
      permutation == pcg_permutation::random_xorshift_multiply_xorshift
   ) {
      static_assert(sizeof(State) == 8u && sizeof(Result) == 8u);
      uint8::raw_type word =
         ((raw_state >> ((raw_state >> 59u) + 5u)) ^ raw_state)
         * 12'605'985'483'714'917'081ull;
      return uint8(word ^ (word >> 43u));
   }
}

}  // namespace detail

template <
   typename State, typename Result, pcg_stream stream,
   pcg_permutation permutation, bool output_after_step = false>
class pcg_engine {
 public:
   using state_type = State;
   using result_type = Result;

 private:
   state_type m_state = detail::pcg_initial_state<state_type>(stream);
   state_type m_increment = detail::pcg_initial_increment<state_type>(stream);

   [[nodiscard]]
   constexpr auto
   stream_increment() const -> state_type {
      if constexpr (stream == pcg_stream::unique) {
         return state_type(reinterpret_cast<__UINTPTR_TYPE__>(this) | 1u);
      }
      return m_increment;
   }

   constexpr void
   step() {
      m_state = m_state * detail::pcg_constants<state_type>::multiplier
                + stream_increment();
   }

 public:
   constexpr pcg_engine() = default;

   constexpr explicit pcg_engine(random_seed initial_state) {
      seed(initial_state);
   }

   constexpr pcg_engine(random_seed initial_state, random_seed initial_sequence)
      requires(stream == pcg_stream::setseq)
   {
      seed(initial_state, initial_sequence);
   }

   constexpr void
   seed() {
      m_state = detail::pcg_initial_state<state_type>(stream);
      m_increment = detail::pcg_initial_increment<state_type>(stream);
   }

   constexpr void
   seed(random_seed initial_state) {
      auto const state = static_cast<state_type>(initial_state);
      if constexpr (stream == pcg_stream::mcg) {
         m_state = state | 1u;
      } else {
         m_state = 0u;
         step();
         m_state += state;
         step();
      }
   }

   constexpr void
   seed(random_seed initial_state, random_seed initial_sequence)
      requires(stream == pcg_stream::setseq)
   {
      auto const state = static_cast<state_type>(initial_state);
      auto const sequence = static_cast<state_type>(initial_sequence);
      m_state = 0u;
      m_increment = (sequence << 1u) | 1u;
      step();
      m_state += state;
      step();
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      if constexpr (output_after_step) {
         step();
         return detail::pcg_output<result_type, state_type, permutation>(
            m_state
         );
      } else {
         state_type const old_state = m_state;
         step();
         return detail::pcg_output<result_type, state_type, permutation>(
            old_state
         );
      }
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
   advance(state_type delta) {
      state_type accumulated_multiplier = 1u;
      state_type accumulated_increment = 0u;
      state_type current_multiplier =
         detail::pcg_constants<state_type>::multiplier;
      state_type current_increment = stream_increment();

      while (delta != 0u) {
         if ((delta & 1u) != 0u) {
            accumulated_multiplier *= current_multiplier;
            accumulated_increment =
               accumulated_increment * current_multiplier + current_increment;
         }
         current_increment *= current_multiplier + 1u;
         current_multiplier *= current_multiplier;
         delta >>= 1u;
      }

      m_state = accumulated_multiplier * m_state + accumulated_increment;
   }

   constexpr void
   backstep(state_type delta) {
      advance(0u - delta);
   }

   constexpr void
   discard(state_type count) {
      advance(count);
   }
};

using pcg32 = pcg_engine<
   uint8, uint4, pcg_stream::setseq,
   pcg_permutation::xorshift_high_random_rotate>;
using pcg32_oneseq = pcg_engine<
   uint8, uint4, pcg_stream::oneseq,
   pcg_permutation::xorshift_high_random_rotate>;
using pcg32_unique = pcg_engine<
   uint8, uint4, pcg_stream::unique,
   pcg_permutation::xorshift_high_random_rotate>;
using pcg32_fast = pcg_engine<
   uint8, uint4, pcg_stream::mcg, pcg_permutation::xorshift_high_random_shift>;

using pcg64 = pcg_engine<
   unsigned __int128, uint8, pcg_stream::setseq,
   pcg_permutation::xorshift_low_random_rotate, true>;
using pcg64_oneseq = pcg_engine<
   unsigned __int128, uint8, pcg_stream::oneseq,
   pcg_permutation::xorshift_low_random_rotate, true>;
using pcg64_unique = pcg_engine<
   unsigned __int128, uint8, pcg_stream::unique,
   pcg_permutation::xorshift_low_random_rotate, true>;
using pcg64_fast = pcg_engine<
   unsigned __int128, uint8, pcg_stream::mcg,
   pcg_permutation::xorshift_low_random_rotate, true>;

}  // namespace cat
