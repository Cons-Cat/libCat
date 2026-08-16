// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

// PCG ("Permuted Congruential Generator") is a fast non-cryptographic PRNG
// engine. It is a popular high quality algorithm family that has excellent
// statistical distribution and highly competitive performance. It is libCat's
// idiomatic engine, used by default in e.g. `random()` and `randn()`. It is
// overwhelmingly industry standard at the time of this writing.
//    https://www.pcg-random.org/index.html
//
// This file implements `pcg_engine`, matching pcg-cpp's `pcg_detail::engine`:
//    XSH RR, XSH RS, XSL RR, RXS M XS, and DXSM
//    streams: mcg, oneseq, setseq, unique
// Result and state types are independent. The usual pairing is a state twice
// as wide as the result. `pcg_dxsm_engine<T, stream>` selects DXSM, including
// NumPy PCG64DXSM (`cm_setseq_dxsm_128_64`).
//
// We lack extended engines, i.e. `pcg32_k32` or `pcg64_c64`. Professor Melissa
// O'Neill, their author, discourages their use.
//    https://www.pcg-random.org/posts/on-vignas-pcg-critique.html
//
// The DXSM variation is chosen as our default for the same reasons it is
// Numpy's default.
//    https://dotat.at/@/2023-06-21-pcg64-dxsm.html
//    https://numpy.org/devdocs/reference/random/upgrading-pcg64.html
//
// This code is well tested in `tests/src/test_pcg.cpp`. Classic engines match
// pcg-cpp `test-high` expected files.

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
   double_xorshift_multiply,
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
   static constexpr uint8 cheap_multiplier = multiplier;
};

template <>
struct pcg_constants<unsigned __int128> {
   static constexpr unsigned __int128 multiplier =
      (static_cast<__uint128_t>(2'549'297'995'355'413'924ull) << 64u)
      | 4'865'540'595'714'422'341ull;
   static constexpr unsigned __int128 increment =
      (static_cast<__uint128_t>(6'364'136'223'846'793'005ull) << 64u)
      | 1'442'695'040'888'963'407ull;

   static constexpr unsigned __int128 oneseq_state =
      (static_cast<__uint128_t>(0xb8dc10e1'58a92392ull) << 64u)
      | 0x98046df0'07ec0a53ull;
   static constexpr unsigned __int128 mcg_state = 0xcafef00d'd15ea5e5ull;
   static constexpr unsigned __int128 setseq_state =
      (static_cast<__uint128_t>(0x979c9a98'd8462005ull) << 64u)
      | 0x7d3e9cb6'cfe0549bull;
   static constexpr unsigned __int128 setseq_increment =
      (static_cast<__uint128_t>(1u) << 64u) | 0xda3e39cb'94b95bdbull;
   static constexpr unsigned __int128 cheap_multiplier = 0xda942042'e4dd58b5ull;
};

template <typename T>
consteval auto
pcg_word_bytes() -> idx {
   return sizeof(raw_arithmetic_type<T>);
}

template <typename T>
   requires(pcg_word_bytes<T>() == 4u || pcg_word_bytes<T>() == 8u)
struct pcg_types {
   using result = conditional<pcg_word_bytes<T>() == 4u, uint4, uint8>;
   using state =
      conditional<pcg_word_bytes<T>() == 4u, uint8, unsigned __int128>;
};

template <typename T, typename State, pcg_stream stream>
consteval auto
pcg_default_permutation() -> pcg_permutation {
   if constexpr (sizeof(State) == sizeof(typename pcg_types<T>::result)) {
      return pcg_permutation::random_xorshift_multiply_xorshift;
   } else if constexpr (pcg_word_bytes<T>() == 8u) {
      return pcg_permutation::xorshift_low_random_rotate;
   } else if constexpr (stream == pcg_stream::mcg) {
      return pcg_permutation::xorshift_high_random_shift;
   } else {
      return pcg_permutation::xorshift_high_random_rotate;
   }
}

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
      uint4 const word = uint4(((raw_state >> 18u) ^ raw_state) >> 27u);
      return rotate_right(word, uword(raw_state >> 59u));
   }

   if constexpr (permutation == pcg_permutation::xorshift_high_random_shift) {
      static_assert(sizeof(State) == 8u && sizeof(Result) == 4u);
      return uint4(
         ((raw_state >> 22u) ^ raw_state) >> ((raw_state >> 61u) + 22u)
      );
   }

   if constexpr (permutation == pcg_permutation::xorshift_low_random_rotate) {
      static_assert(sizeof(State) == 16u && sizeof(Result) == 8u);
      uint8 const word = uint8((raw_state >> 64u) ^ raw_state);
      return rotate_right(word, uword(raw_state >> 122u));
   }

   if constexpr (
      permutation == pcg_permutation::random_xorshift_multiply_xorshift
   ) {
      static_assert(sizeof(State) == 8u && sizeof(Result) == 8u);
      uint8 word = uint8(
         ((raw_state >> ((raw_state >> 59u) + 5u)) ^ raw_state)
         * 12'605'985'483'714'917'081ull
      );
      return word ^ (word >> 43u);
   }

   if constexpr (permutation == pcg_permutation::double_xorshift_multiply) {
      static_assert(sizeof(State) == sizeof(Result) * 2u);
      if constexpr (sizeof(Result) == 4u) {
         uint4::raw_type hi = uint4::raw_type(raw_state >> 32u);
         uint4::raw_type lo = uint4::raw_type(raw_state) | 1u;
         hi ^= hi >> 16u;
         hi *= uint4::raw_type(
            make_raw_arithmetic(pcg_constants<State>::cheap_multiplier)
         );
         hi ^= hi >> 24u;
         hi *= lo;
         return uint4(hi);
      } else {
         uint8::raw_type hi = uint8::raw_type(raw_state >> 64u);
         uint8::raw_type lo = uint8::raw_type(raw_state) | 1u;
         hi ^= hi >> 32u;
         hi *= uint8::raw_type(
            make_raw_arithmetic(pcg_constants<State>::cheap_multiplier)
         );
         hi ^= hi >> 48u;
         hi *= lo;
         return uint8(hi);
      }
   }
}

template <typename State>
[[nodiscard]]
constexpr auto
pcg_distance(
   State current, State target, State multiplier, State increment, State mask
) -> State {
   bool const mcg = increment == 0u;
   State bit = mcg ? 4u : 1u;
   State distance = 0u;
   while ((current & mask) != (target & mask)) {
      if ((current & bit) != (target & bit)) {
         current = current * multiplier + increment;
         distance |= bit;
      }
      bit <<= 1u;
      increment = (multiplier + 1u) * increment;
      multiplier *= multiplier;
   }
   return mcg ? distance >> 2u : distance;
}

}  // namespace detail

template <
   typename T, pcg_stream stream = pcg_stream::setseq,
   typename State = typename detail::pcg_types<T>::state,
   pcg_permutation permutation =
      detail::pcg_default_permutation<T, State, stream>(),
   bool output_previous = sizeof(State) <= 8u,
   bool use_cheap_multiplier = false>
   requires(
      detail::pcg_word_bytes<T>() == 4u || detail::pcg_word_bytes<T>() == 8u
   )
class pcg_engine {
 public:
   using result_type = typename detail::pcg_types<T>::result;
   using state_type = State;

 private:
   state_type m_state = detail::pcg_initial_state<state_type>(stream);
   state_type m_increment = detail::pcg_initial_increment<state_type>(stream);

 public:
   [[nodiscard]]
   constexpr auto
   increment() const -> state_type {
      if constexpr (stream == pcg_stream::unique) {
         return state_type(uintptr<pcg_engine const>(this).raw | 1u);
      }
      return m_increment;
   }

   [[nodiscard]]
   static constexpr auto
   multiplier() -> state_type {
      if constexpr (use_cheap_multiplier) {
         return state_type(detail::pcg_constants<state_type>::cheap_multiplier);
      }
      return detail::pcg_constants<state_type>::multiplier;
   }

   [[nodiscard]]
   constexpr auto
   stream_id() const -> state_type {
      return increment() >> 1u;
   }

   [[nodiscard]]
   static consteval auto
   period_pow2() -> idx {
      return sizeof(state_type) * 8u - (stream == pcg_stream::mcg ? 2u : 0u);
   }

   [[nodiscard]]
   static consteval auto
   streams_pow2() -> idx {
      if constexpr (stream == pcg_stream::setseq) {
         return sizeof(state_type) * 8u - 1u;
      }
      if constexpr (stream == pcg_stream::unique) {
         return sizeof(void*) * 8u - 1u;
      }
      return 0u;
   }

 private:
   constexpr void
   step() {
      m_state = m_state * multiplier() + increment();
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
      auto const state = state_type(initial_state);
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
      auto const state = state_type(initial_state);
      auto const sequence = state_type(initial_sequence);
      m_state = 0u;
      m_increment = (sequence << 1u) | 1u;
      step();
      m_state += state;
      step();
   }

   constexpr void
   set_stream(state_type sequence)
      requires(stream == pcg_stream::setseq)
   {
      m_increment = (sequence << 1u) | 1u;
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      if constexpr (output_previous) {
         state_type const old_state = m_state;
         step();
         return detail::pcg_output<result_type, state_type, permutation>(
            old_state
         );
      } else {
         step();
         return detail::pcg_output<result_type, state_type, permutation>(
            m_state
         );
      }
   }

   [[nodiscard]]
   constexpr auto
   operator()(result_type bound) -> result_type {
      if (bound == 0u) {
         return (*this)();
      }
      result_type const threshold = (result_type(0u) - bound) % bound;
      while (true) {
         result_type const value = (*this)();
         if (value >= threshold) {
            return value % bound;
         }
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

   // Skip this many outputs.
   constexpr void
   discard(state_type count) {
      state_type accumulated_multiplier = 1u;
      state_type accumulated_increment = 0u;
      state_type current_multiplier = multiplier();
      state_type current_increment = increment();

      while (count != 0u) {
         if ((count & 1u) != 0u) {
            accumulated_multiplier *= current_multiplier;
            accumulated_increment =
               accumulated_increment * current_multiplier + current_increment;
         }
         current_increment *= current_multiplier + 1u;
         current_multiplier *= current_multiplier;
         count >>= 1u;
      }

      m_state = accumulated_multiplier * m_state + accumulated_increment;
   }

   // Skip this many outputs backwards.
   constexpr void
   backstep(state_type delta) {
      discard(state_type(0u) - delta);
   }

   [[nodiscard]]
   constexpr auto
   wrapped() const -> bool {
      if constexpr (stream == pcg_stream::mcg) {
         return (m_state >> 2u) == 0u;
      }
      return m_state == 0u;
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(pcg_engine const& left, pcg_engine const& right) -> bool {
      return left.m_state == right.m_state
             && left.increment() == right.increment();
   }

   [[nodiscard]]
   friend constexpr auto
   operator-(pcg_engine const& left, pcg_engine const& right) -> state_type {
      if (left.increment() == right.increment()) {
         return detail::pcg_distance(
            right.m_state, left.m_state, left.multiplier(), left.increment(),
            state_type(0u) - 1u
         );
      }
      state_type left_diff =
         left.increment() + (left.multiplier() - 1u) * left.m_state;
      state_type right_diff =
         right.increment() + (right.multiplier() - 1u) * right.m_state;
      if ((left_diff & 3u) != (right_diff & 3u)) {
         right_diff = state_type(0u) - right_diff;
      }
      return detail::pcg_distance(
         right_diff, left_diff, right.multiplier(), state_type(0u),
         state_type(0u) - 1u
      );
   }
};

template <typename T, pcg_stream stream = pcg_stream::setseq>
   requires(
      detail::pcg_word_bytes<T>() == 4u || detail::pcg_word_bytes<T>() == 8u
   )
class pcg_dxsm_engine : public pcg_engine<
                           T, stream, typename detail::pcg_types<T>::state,
                           pcg_permutation::double_xorshift_multiply, true,
                           detail::pcg_word_bytes<T>() == 8u> {
   using base = pcg_engine<
      T, stream, typename detail::pcg_types<T>::state,
      pcg_permutation::double_xorshift_multiply, true,
      detail::pcg_word_bytes<T>() == 8u>;

 public:
   using base::base;
};

}  // namespace cat
