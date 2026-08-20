// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

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
// The DXSM variation is chosen as our default. NumPy offers PCG64DXSM as an
// upgrade, while its `default_rng` continues to use PCG64.
//    https://dotat.at/@/2023-06-21-pcg64-dxsm.html
//    https://numpy.org/devdocs/reference/random/upgrading-pcg64.html
//
// This code is well tested in `tests/src/test_pcg.cpp`. Classic engines match
// pcg-cpp `test-high` expected files.

#include <cat/random>

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

// PCG requires wide state values, which cannot be stored in singular SIMD
// lanes. This class emulates that in a SIMD fashion.
template <is_simd Word>
class pcg_simd_state {
 public:
   using word_type = Word;
   using lane_type = word_type::value_type;
   using lane_raw = raw_arithmetic_type<lane_type>;
   using scalar_type =
      conditional<sizeof(lane_type) == 4u, uint8, unsigned __int128>;
   using mask_type = word_type::mask_type;

 private:
   static constexpr idx word_bits = sizeof(lane_type) * 8u;
   word_type m_low = 0u;
   word_type m_high = 0u;

   [[nodiscard]]
   static constexpr auto
   multiply_high(word_type left, word_type right) -> word_type {
      constexpr lane_type half_bits = sizeof(lane_type) == 4u ? 16u : 32u;
      constexpr lane_type half_mask =
         sizeof(lane_type) == 4u ? 0xffffu : 0xffffffffull;
      word_type const left_low = left & half_mask;
      word_type const left_high = left >> half_bits;
      word_type const right_low = right & half_mask;
      word_type const right_high = right >> half_bits;
      word_type const low_product = left_low * right_low;
      word_type const middle =
         left_high * right_low + (low_product >> half_bits);
      word_type const middle_low = middle & half_mask;
      word_type const middle_high = middle >> half_bits;
      word_type const cross = middle_low + left_low * right_high;
      return left_high * right_high + middle_high + (cross >> half_bits);
   }

 public:
   constexpr pcg_simd_state() = default;

   constexpr pcg_simd_state(scalar_type value)
       : m_low(lane_type(lane_raw(make_raw_arithmetic(value)))),
         m_high(
            lane_type(lane_raw(make_raw_arithmetic(value >> word_bits.raw)))
         ) {
   }

   template <is_unsigned_integral Value>
      requires(!is_same<Value, scalar_type>)
   constexpr pcg_simd_state(Value value) : pcg_simd_state(scalar_type(value)) {
   }

   constexpr pcg_simd_state(word_type low) : m_low(low) {
   }

   constexpr pcg_simd_state(word_type low, word_type high)
       : m_low(low), m_high(high) {
   }

   [[nodiscard]]
   constexpr auto
   low() const -> word_type {
      return m_low;
   }

   [[nodiscard]]
   constexpr auto
   high() const -> word_type {
      return m_high;
   }

   [[nodiscard]]
   constexpr auto
   equal_lanes(pcg_simd_state const& operand) const -> mask_type {
      return m_low.equal_lanes(operand.m_low)
             & m_high.equal_lanes(operand.m_high);
   }

   [[nodiscard]]
   static constexpr auto
   select(mask_type mask, pcg_simd_state on_true, pcg_simd_state on_false)
      -> pcg_simd_state {
      return {
         simd_select(mask, on_true.m_low, on_false.m_low),
         simd_select(mask, on_true.m_high, on_false.m_high),
      };
   }

   constexpr auto
   operator+=(pcg_simd_state operand) -> pcg_simd_state& {
      word_type const low = m_low + operand.m_low;
      m_high += operand.m_high;
      m_high += simd_select(low < m_low, word_type(1u), word_type(0u));
      m_low = low;
      return *this;
   }

   constexpr auto
   operator*=(pcg_simd_state operand) -> pcg_simd_state& {
      m_high = m_high * operand.m_low + m_low * operand.m_high
               + multiply_high(m_low, operand.m_low);
      m_low *= operand.m_low;
      return *this;
   }

   constexpr auto
   operator&=(pcg_simd_state operand) -> pcg_simd_state& {
      m_low &= operand.m_low;
      m_high &= operand.m_high;
      return *this;
   }

   constexpr auto
   operator|=(pcg_simd_state operand) -> pcg_simd_state& {
      m_low |= operand.m_low;
      m_high |= operand.m_high;
      return *this;
   }

   constexpr auto
   operator^=(pcg_simd_state operand) -> pcg_simd_state& {
      m_low ^= operand.m_low;
      m_high ^= operand.m_high;
      return *this;
   }

   constexpr auto
   operator>>=(idx count) -> pcg_simd_state& {
      *this = *this >> count;
      return *this;
   }

   constexpr auto
   operator<<=(idx count) -> pcg_simd_state& {
      *this = *this << count;
      return *this;
   }

   [[nodiscard]]
   friend constexpr auto
   operator+(pcg_simd_state left, pcg_simd_state right) -> pcg_simd_state {
      left += right;
      return left;
   }

   [[nodiscard]]
   friend constexpr auto
   operator-(pcg_simd_state left, pcg_simd_state right) -> pcg_simd_state {
      word_type const low = left.m_low - right.m_low;
      left.m_high -= right.m_high;
      left.m_high -=
         simd_select(left.m_low < right.m_low, word_type(1u), word_type(0u));
      left.m_low = low;
      return left;
   }

   [[nodiscard]]
   friend constexpr auto
   operator*(pcg_simd_state left, pcg_simd_state right) -> pcg_simd_state {
      left *= right;
      return left;
   }

   [[nodiscard]]
   friend constexpr auto
   operator&(pcg_simd_state left, pcg_simd_state right) -> pcg_simd_state {
      left &= right;
      return left;
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(pcg_simd_state left, pcg_simd_state right) -> pcg_simd_state {
      left |= right;
      return left;
   }

   [[nodiscard]]
   friend constexpr auto
   operator^(pcg_simd_state left, pcg_simd_state right) -> pcg_simd_state {
      left ^= right;
      return left;
   }

   [[nodiscard]]
   friend constexpr auto
   operator>>(pcg_simd_state value, idx count) -> pcg_simd_state {
      if (count == 0u) {
         return value;
      }
      if (count < word_bits) {
         auto const shift = lane_type(count.raw);
         auto const inverse = lane_type(word_bits.raw - count.raw);
         return {
            (value.m_low >> shift) | (value.m_high << inverse),
            value.m_high >> shift,
         };
      }
      if (count < word_bits * 2u) {
         return {
            value.m_high >> lane_type(count.raw - word_bits.raw),
         };
      }
      return {};
   }

   [[nodiscard]]
   friend constexpr auto
   operator<<(pcg_simd_state value, idx count) -> pcg_simd_state {
      if (count == 0u) {
         return value;
      }
      if (count < word_bits) {
         auto const shift = lane_type(count.raw);
         auto const inverse = lane_type(word_bits.raw - count.raw);
         return {
            value.m_low << shift,
            (value.m_high << shift) | (value.m_low >> inverse),
         };
      }
      if (count < word_bits * 2u) {
         return {
            word_type(0u),
            value.m_low << lane_type(count.raw - word_bits.raw),
         };
      }
      return {};
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(pcg_simd_state const& left, pcg_simd_state const& right) -> bool {
      return left.m_low == right.m_low && left.m_high == right.m_high;
   }
};

template <typename T>
struct pcg_types;

template <typename T>
   requires(!is_simd<T> && (sizeof(T) == 4u || sizeof(T) == 8u))
struct pcg_types<T> {
   using result = conditional<sizeof(T) == 4u, uint4, uint8>;
   // PCG state is always wider than the result value.
   using state = conditional<sizeof(T) == 4u, uint8, unsigned __int128>;
};

template <typename Lane, typename Abi>
   requires(sizeof(Lane) == 4u || sizeof(Lane) == 8u)
struct pcg_types<simd<Lane, Abi>> {
   using result_lane = conditional<sizeof(Lane) == 4u, uint4, uint8>;
   using result =
      simd<result_lane, typename Abi::template make_abi_type<result_lane>>;
   using state = pcg_simd_state<result>;
};

template <typename T, typename State, pcg_stream stream>
consteval auto
pcg_default_permutation() -> pcg_permutation {
   if constexpr (is_simd<T>) {
      if constexpr (is_same<State, typename pcg_types<T>::result>) {
         static_assert(sizeof(typename T::value_type) == 8u);
         return pcg_permutation::random_xorshift_multiply_xorshift;
      } else if constexpr (sizeof(typename T::value_type) == 8u) {
         return pcg_permutation::xorshift_low_random_rotate;
      } else if constexpr (stream == pcg_stream::mcg) {
         return pcg_permutation::xorshift_high_random_shift;
      } else {
         return pcg_permutation::xorshift_high_random_rotate;
      }
   } else if constexpr (
      sizeof(State) == sizeof(typename pcg_types<T>::result)
   ) {
      return pcg_permutation::random_xorshift_multiply_xorshift;
   } else if constexpr (sizeof(T) == 8u) {
      return pcg_permutation::xorshift_low_random_rotate;
   } else if constexpr (stream == pcg_stream::mcg) {
      return pcg_permutation::xorshift_high_random_shift;
   } else {
      return pcg_permutation::xorshift_high_random_rotate;
   }
}

template <typename State, pcg_stream stream>
inline constexpr State pcg_initial_state = [] {
   if constexpr (stream == pcg_stream::mcg) {
      return pcg_constants<State>::mcg_state;
   } else if constexpr (stream == pcg_stream::setseq) {
      return pcg_constants<State>::setseq_state;
   } else {
      return pcg_constants<State>::oneseq_state;
   }
}();

template <typename State, pcg_stream stream>
inline constexpr State pcg_initial_increment = [] {
   if constexpr (stream == pcg_stream::setseq) {
      return pcg_constants<State>::setseq_increment;
   } else if constexpr (stream == pcg_stream::oneseq) {
      return pcg_constants<State>::increment;
   } else {
      return State(0u);
   }
}();

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
   typename State = detail::pcg_types<T>::state,
   pcg_permutation permutation =
      detail::pcg_default_permutation<T, State, stream>(),
   bool output_previous =
      sizeof(State) <= 8u
      || (is_simd<T> && (sizeof(detail::random_scalar<T>) == 4u || is_same<State, typename detail::pcg_types<T>::result>)),
   bool use_cheap_multiplier = false>
   requires(
      sizeof(detail::random_scalar<T>) == 4u
      || sizeof(detail::random_scalar<T>) == 8u
   )
class pcg_engine {
 public:
   using result_type = detail::pcg_types<T>::result;
   using state_type = State;

 private:
   state_type m_state = detail::pcg_initial_state<state_type, stream>;
   state_type m_increment = detail::pcg_initial_increment<state_type, stream>;

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
   constexpr auto
   state() const -> state_type {
      return m_state;
   }

   [[nodiscard]]
   static consteval auto
   period_pow2() -> idx {
      return (sizeof(state_type) * 8u) - (stream == pcg_stream::mcg ? 2u : 0u);
   }

   [[nodiscard]]
   static consteval auto
   streams_pow2() -> idx {
      if constexpr (stream == pcg_stream::setseq) {
         return (sizeof(state_type) * 8u) - 1u;
      }
      if constexpr (stream == pcg_stream::unique) {
         return (sizeof(void*) * 8u) - 1u;
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
      m_state = detail::pcg_initial_state<state_type, stream>;
      m_increment = detail::pcg_initial_increment<state_type, stream>;
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

   constexpr void
   set_state(state_type state) {
      m_state = state;
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

template <
   typename Lane, typename Abi, pcg_stream stream, typename State,
   pcg_permutation permutation, bool output_previous, bool use_cheap_multiplier>
class pcg_engine<
   simd<Lane, Abi>, stream, State, permutation, output_previous,
   use_cheap_multiplier> {
 public:
   using result_type = detail::pcg_types<simd<Lane, Abi>>::result;
   using state_type = State;

 private:
   using lane_type = result_type::value_type;
   using lane_raw = raw_arithmetic_type<lane_type>;
   using expected_state = detail::pcg_simd_state<result_type>;
   using mask_type = result_type::mask_type;
   static constexpr bool equal_width_state = is_same<state_type, result_type>;
   using scalar_state = conditional<
      equal_width_state, lane_type, typename expected_state::scalar_type>;
   static constexpr idx word_bits = sizeof(lane_type) * 8u;
   static constexpr idx state_bits =
      equal_width_state ? word_bits : word_bits * 2u;

   static_assert(
      (is_same<state_type, expected_state>
       && permutation != pcg_permutation::random_xorshift_multiply_xorshift)
      || (equal_width_state && word_bits == 64u && permutation == pcg_permutation::random_xorshift_multiply_xorshift)
   );

   state_type m_state =
      state_type(detail::pcg_initial_state<scalar_state, stream>);
   state_type m_increment =
      state_type(detail::pcg_initial_increment<scalar_state, stream>);
   result_type m_lane_offsets = simd_iota<result_type>(0u) * 2u;

   [[nodiscard]]
   static constexpr auto
   rotate_right(result_type value, result_type count) -> result_type {
      constexpr auto mask = lane_type(word_bits - 1u);
      return (value >> count) | (value << ((result_type(0u) - count) & mask));
   }

   [[nodiscard]]
   static constexpr auto
   select(mask_type mask, state_type on_true, state_type on_false)
      -> state_type {
      if constexpr (equal_width_state) {
         return simd_select(mask, on_true, on_false);
      } else {
         return state_type::select(mask, on_true, on_false);
      }
   }

   [[nodiscard]]
   static constexpr auto
   output(state_type state) -> result_type {
      if constexpr (
         permutation == pcg_permutation::xorshift_high_random_rotate
      ) {
         result_type const word = (((state >> 18u) ^ state) >> 27u).low();
         return rotate_right(word, state.high() >> 27u);
      }

      if constexpr (
         permutation == pcg_permutation::xorshift_high_random_shift
      ) {
         state_type const mixed = (state >> 22u) ^ state;
         result_type const shift = (state.high() >> 29u) + 22u;
         return (mixed.low() >> shift)
                | (mixed.high() << (result_type(lane_type(32u)) - shift));
      }

      if constexpr (
         permutation == pcg_permutation::xorshift_low_random_rotate
      ) {
         return rotate_right(state.high() ^ state.low(), state.high() >> 58u);
      }

      if constexpr (
         permutation == pcg_permutation::random_xorshift_multiply_xorshift
      ) {
         result_type word = ((state >> ((state >> 59u) + 5u)) ^ state)
                            * lane_type(12'605'985'483'714'917'081ull);
         return word ^ (word >> 43u);
      }

      if constexpr (permutation == pcg_permutation::double_xorshift_multiply) {
         result_type word = state.high();
         if constexpr (word_bits == 32u) {
            word ^= word >> 16u;
            word *= lane_type(lane_raw(make_raw_arithmetic(
               detail::pcg_constants<scalar_state>::cheap_multiplier
            )));
            word ^= word >> 24u;
         } else {
            word ^= word >> 32u;
            word *= lane_type(lane_raw(make_raw_arithmetic(
               detail::pcg_constants<scalar_state>::cheap_multiplier
            )));
            word ^= word >> 48u;
         }
         return word * (state.low() | 1u);
      }
   }

   constexpr void
   step(mask_type active) {
      state_type const next = m_state * multiplier() + increment();
      m_state = select(active, next, m_state);
   }

   [[nodiscard]]
   constexpr auto
   generate(mask_type active) -> result_type {
      if constexpr (output_previous) {
         state_type const old_state = m_state;
         step(active);
         return output(old_state);
      } else {
         step(active);
         return output(m_state);
      }
   }

   [[nodiscard]]
   static constexpr auto
   distance(
      state_type current, state_type target, state_type multiplier_value,
      state_type increment_value
   ) -> state_type {
      state_type bit = stream == pcg_stream::mcg ? 4u : 1u;
      state_type result = 0u;
      for (idx index = 0u; index < state_bits; ++index) {
         mask_type const differs = !(current & bit).equal_lanes(target & bit);
         state_type const advanced =
            current * multiplier_value + increment_value;
         current = select(differs, advanced, current);
         result |= select(differs, bit, state_type(0u));
         bit <<= 1u;
         increment_value = (multiplier_value + 1u) * increment_value;
         multiplier_value *= multiplier_value;
      }
      if constexpr (stream == pcg_stream::mcg) {
         return result >> 2u;
      }
      return result;
   }

 public:
   constexpr pcg_engine() = default;

   constexpr explicit pcg_engine(random_seed initial_state_value) {
      seed(initial_state_value);
   }

   constexpr pcg_engine(
      random_seed initial_state_value, random_seed initial_sequence
   )
      requires(stream == pcg_stream::setseq)
   {
      seed(initial_state_value, initial_sequence);
   }

   [[nodiscard]]
   constexpr auto
   increment() const -> state_type {
      if constexpr (stream == pcg_stream::unique) {
         scalar_state const base = uintptr<pcg_engine const>(this).raw | 1u;
         return state_type(base) + state_type(m_lane_offsets);
      }
      return m_increment;
   }

   [[nodiscard]]
   static constexpr auto
   multiplier() -> state_type {
      if constexpr (use_cheap_multiplier) {
         return state_type(
            detail::pcg_constants<scalar_state>::cheap_multiplier
         );
      }
      return state_type(detail::pcg_constants<scalar_state>::multiplier);
   }

   [[nodiscard]]
   constexpr auto
   stream_id() const -> state_type {
      return increment() >> 1u;
   }

   [[nodiscard]]
   constexpr auto
   state() const -> state_type {
      return m_state;
   }

   [[nodiscard]]
   static consteval auto
   period_pow2() -> idx {
      return idx(state_bits.raw - (stream == pcg_stream::mcg ? 2u : 0u));
   }

   [[nodiscard]]
   static consteval auto
   streams_pow2() -> idx {
      if constexpr (stream == pcg_stream::setseq) {
         return idx(state_bits.raw - 1u);
      }
      if constexpr (stream == pcg_stream::unique) {
         return (sizeof(void*) * 8u) - 1u;
      }
      return 0u;
   }

   constexpr void
   seed() {
      m_state = state_type(detail::pcg_initial_state<scalar_state, stream>);
      m_increment =
         state_type(detail::pcg_initial_increment<scalar_state, stream>);
   }

   constexpr void
   seed(random_seed initial_state_value) {
      state_type const state =
         state_type(static_cast<scalar_state>(initial_state_value))
         + state_type(simd_iota<result_type>(0u));
      if constexpr (stream == pcg_stream::mcg) {
         m_state = state | 1u;
      } else {
         m_state = 0u;
         step(mask_type(true));
         m_state += state;
         step(mask_type(true));
      }
   }

   constexpr void
   seed(random_seed initial_state_value, random_seed initial_sequence)
      requires(stream == pcg_stream::setseq)
   {
      state_type const offsets(simd_iota<result_type>(0u));
      state_type const state =
         state_type(static_cast<scalar_state>(initial_state_value)) + offsets;
      state_type const sequence =
         state_type(static_cast<scalar_state>(initial_sequence)) + offsets;
      m_state = 0u;
      m_increment = (sequence << 1u) | 1u;
      step(mask_type(true));
      m_state += state;
      step(mask_type(true));
   }

   constexpr void
   set_stream(state_type sequence)
      requires(stream == pcg_stream::setseq)
   {
      m_increment = (sequence << 1u) | 1u;
   }

   constexpr void
   set_state(state_type state) {
      m_state = state;
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      return generate(mask_type(true));
   }

   [[nodiscard]]
   constexpr auto
   operator()(result_type bound) -> result_type {
      result_type result = 0u;
      result_type const zero = 0u;
      mask_type const unbounded = bound.equal_lanes(zero);
      result_type const divisor =
         simd_select(unbounded, result_type(1u), bound);
      result_type const threshold = (zero - divisor) % divisor;
      mask_type pending(true);
      while (pending.any_of()) {
         result_type const value = generate(pending);
         mask_type const accepted =
            pending & (unbounded | (value >= threshold));
         result_type const bounded = value % divisor;
         result = simd_select(
            accepted, simd_select(unbounded, value, bounded), result
         );
         pending &= !accepted;
      }
      return result;
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type {
      return 0u;
   }

   [[nodiscard]]
   static constexpr auto
   max() -> result_type {
      return lane_type::max();
   }

   constexpr void
   discard(state_type count) {
      state_type accumulated_multiplier = 1u;
      state_type accumulated_increment = 0u;
      state_type current_multiplier = multiplier();
      state_type current_increment = increment();

      for (idx index = 0u; index < state_bits; ++index) {
         mask_type const active =
            (count & state_type(1u)).equal_lanes(state_type(1u));
         state_type const next_multiplier =
            accumulated_multiplier * current_multiplier;
         state_type const next_increment =
            accumulated_increment * current_multiplier + current_increment;
         accumulated_multiplier =
            select(active, next_multiplier, accumulated_multiplier);
         accumulated_increment =
            select(active, next_increment, accumulated_increment);
         current_increment *= current_multiplier + 1u;
         current_multiplier *= current_multiplier;
         count >>= 1u;
      }

      m_state = accumulated_multiplier * m_state + accumulated_increment;
   }

   constexpr void
   backstep(state_type delta) {
      discard(state_type(0u) - delta);
   }

   [[nodiscard]]
   constexpr auto
   wrapped() const -> bool {
      if constexpr (stream == pcg_stream::mcg) {
         return (m_state >> 2u) == state_type(0u);
      }
      return m_state == state_type(0u);
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
         return distance(
            right.m_state, left.m_state, left.multiplier(), left.increment()
         );
      }
      state_type const left_diff =
         left.increment() + (left.multiplier() - 1u) * left.m_state;
      state_type right_diff =
         right.increment() + (right.multiplier() - 1u) * right.m_state;
      mask_type const negate =
         !(left_diff & state_type(3u)).equal_lanes(right_diff & state_type(3u));
      right_diff = select(negate, state_type(0u) - right_diff, right_diff);
      return distance(
         right_diff, left_diff, right.multiplier(), state_type(0u)
      );
   }
};

template <typename T, pcg_stream stream = pcg_stream::setseq>
   requires(
      sizeof(detail::random_scalar<T>) == 4u
      || sizeof(detail::random_scalar<T>) == 8u
   )
class pcg_dxsm_engine : public pcg_engine<
                           T, stream, typename detail::pcg_types<T>::state,
                           pcg_permutation::double_xorshift_multiply, true,
                           sizeof(detail::random_scalar<T>) == 8u> {
   using base = pcg_engine<
      T, stream, typename detail::pcg_types<T>::state,
      pcg_permutation::double_xorshift_multiply, true,
      sizeof(detail::random_scalar<T>) == 8u>;

 public:
   using base::base;
};

}  // namespace cat
