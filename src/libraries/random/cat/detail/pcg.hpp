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
// It should be noted that PCG family performs subpar in some SIMD contexts, so
// an alternative engine like `cat::xoshiro_engine` may be preferred for
// batching PRNG.
//
// libCat API extensions:
//
// `operator()(bound)` draws from `[0, bound)`, with zero requesting the full
// result range. `operator()(minimum, maximum)` uses the inclusive interval
// `[minimum, maximum]`. SIMD bounds and results operate lane by lane.
//
//    pcg_dxsm_engine<uint8> engine(42u, 54u);
//    uint8 die_roll = engine(6u);
//
// The default `setseq` stream accepts an initial sequence. `oneseq` has one
// fixed stream, `unique` derives it from the engine address, and `mcg` uses the
// multiplicative recurrence with one quarter of the usual period.
//
//    using selectable_engine = pcg_dxsm_engine<uint8, pcg_stream::setseq>;
//    using fixed_engine = pcg_dxsm_engine<uint8, pcg_stream::oneseq>;
//    selectable_engine selected(42u, 7u);
//
// `discard(n)` is the standard engine operation and uses logarithmic exact
// advance.
//
// SIMD construction offsets the seed and selectable sequence by lane. Bounded
// draw and discard operations are lane-wise.
//
//    pcg_dxsm_engine<uint8x4> wide(42u, 54u);
//    auto bounded = wide(uint8x4(10u));
//
// This code is well tested in `tests/src/test_pcg.cpp`. Classic engines match
// pcg-cpp `test-high` expected files.

#include <cat/random>

#include "./random_batch.hpp"

namespace cat {

enum class pcg_stream : uint1::raw_type {
   setseq,
   oneseq,
   unique,
   mcg,
};

namespace detail {

enum class pcg_permutation : uint1::raw_type {
   xorshift_high_random_rotate,
   xorshift_high_random_shift,
   xorshift_low_random_rotate,
   random_xorshift_multiply_xorshift,
   double_xorshift_multiply,
};

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

template <
   is_simd Word,
   bool use_native_state = sizeof(typename Word::value_type) == 4u>
class pcg_simd_state;

template <is_simd Word>
class pcg_simd_state<Word, true> {
 public:
   using word_type = Word;
   using lane_type = word_type::value_type;
   using lane_raw = raw_arithmetic_type<lane_type>;
   using scalar_type = uint8;
   using mask_type = word_type::mask_type;

 private:
   using wide_lane = uint8;
   using wide_abi = conditional<
      word_type::abi_type::lanes % 2u == 0u,
      typename word_type::abi_type::template make_abi_type<wide_lane>,
      ::cat::simd_abi::fixed_size<
         wide_lane, (word_type::abi_type::lanes + 1u) / 2u>>;
   using wide_word = simd<wide_lane, wide_abi>;
   static constexpr idx wide_lanes = wide_word::abi_type::lanes;
   static_assert(word_type::abi_type::lanes <= wide_lanes * 2u);

   array<wide_word, 2u> m_words;

   [[nodiscard]]
   constexpr auto
   lane(idx index) const -> scalar_type {
      idx const word = index / wide_lanes;
      idx const lane = index % wide_lanes;
      return m_words[word][lane];
   }

   constexpr void
   set_lane(idx index, scalar_type value) {
      idx const word = index / wide_lanes;
      idx const lane = index % wide_lanes;
      m_words[word].set_lane(lane, value);
   }

   [[nodiscard]]
   static constexpr auto
   pack(word_type low, word_type high) -> pcg_simd_state {
      pcg_simd_state result;
      for (idx lane = 0u; lane < word_type::abi_type::lanes; ++lane) {
         uint8 const value = uint8(low[lane]) | (uint8(high[lane]) << 32u);
         result.set_lane(lane, value);
      }
      return result;
   }

   template <pcg_permutation permutation>
   [[nodiscard]]
   static constexpr auto
   output_word(wide_word state) -> wide_word {
      wide_word const mask = 0xffffffffull;
      if constexpr (
         permutation == pcg_permutation::xorshift_high_random_rotate
      ) {
         wide_word const word = (((state >> 18u) ^ state) >> 27u) & mask;
         wide_word const count = state >> 59u;
         return ((word >> count)
                 | (word << ((wide_word(0u) - count) & wide_word(31u))))
                & mask;
      }
      if constexpr (
         permutation == pcg_permutation::xorshift_high_random_shift
      ) {
         wide_word const count = (state >> 61u) + 22u;
         return (((state >> 22u) ^ state) >> count) & mask;
      }
      if constexpr (permutation == pcg_permutation::double_xorshift_multiply) {
         wide_word word = state >> 32u;
         word ^= word >> 16u;
         word = (word
                 * wide_word(uint8(
                    make_raw_arithmetic(pcg_constants<uint8>::cheap_multiplier)
                 )))
                & mask;
         word ^= word >> 24u;
         return (word * (state | 1u)) & mask;
      }
   }

 public:
   constexpr pcg_simd_state() = default;

   constexpr pcg_simd_state(scalar_type value)
       : m_words{wide_word(value), wide_word(value)} {
   }

   template <is_unsigned_integral Value>
      requires(!is_same<Value, scalar_type>)
   constexpr pcg_simd_state(Value value) : pcg_simd_state(scalar_type(value)) {
   }

   constexpr pcg_simd_state(word_type low) : pcg_simd_state(pack(low, 0u)) {
   }

   constexpr pcg_simd_state(word_type low, word_type high)
       : pcg_simd_state(pack(low, high)) {
   }

   [[nodiscard]]
   constexpr auto
   low() const -> word_type {
      word_type result;
      for (idx lane = 0u; lane < word_type::abi_type::lanes; ++lane) {
         result.set_lane(
            lane, lane_type(lane_raw(make_raw_arithmetic(this->lane(lane))))
         );
      }
      return result;
   }

   [[nodiscard]]
   constexpr auto
   high() const -> word_type {
      word_type result;
      for (idx lane = 0u; lane < word_type::abi_type::lanes; ++lane) {
         result.set_lane(
            lane,
            lane_type(lane_raw(make_raw_arithmetic(this->lane(lane) >> 32u)))
         );
      }
      return result;
   }

   template <pcg_permutation permutation>
   [[nodiscard]]
   constexpr auto
   output() const -> word_type {
      array<wide_word, 2u> const words = {
         output_word<permutation>(m_words[0u]),
         output_word<permutation>(m_words[1u]),
      };
      word_type result;
      for (idx lane = 0u; lane < word_type::abi_type::lanes; ++lane) {
         idx const word = lane / wide_lanes;
         idx const word_lane = lane % wide_lanes;
         result.set_lane(
            lane,
            lane_type(lane_raw(make_raw_arithmetic(words[word][word_lane])))
         );
      }
      return result;
   }

   [[nodiscard]]
   constexpr auto
   equal_lanes(pcg_simd_state const& operand) const -> mask_type {
      mask_type result;
      for (idx lane = 0u; lane < word_type::abi_type::lanes; ++lane) {
         result.set_lane(lane, this->lane(lane) == operand.lane(lane));
      }
      return result;
   }

   [[nodiscard]]
   static constexpr auto
   select(mask_type mask, pcg_simd_state on_true, pcg_simd_state on_false)
      -> pcg_simd_state {
      pcg_simd_state result;
      for (idx lane = 0u; lane < word_type::abi_type::lanes; ++lane) {
         result.set_lane(
            lane, mask[lane] ? on_true.lane(lane) : on_false.lane(lane)
         );
      }
      return result;
   }

   constexpr auto
   operator+=(pcg_simd_state operand) -> pcg_simd_state& {
      m_words[0u] += operand.m_words[0u];
      m_words[1u] += operand.m_words[1u];
      return *this;
   }

   constexpr auto
   operator*=(pcg_simd_state operand) -> pcg_simd_state& {
      m_words[0u] *= operand.m_words[0u];
      m_words[1u] *= operand.m_words[1u];
      return *this;
   }

   constexpr auto
   operator&=(pcg_simd_state operand) -> pcg_simd_state& {
      m_words[0u] &= operand.m_words[0u];
      m_words[1u] &= operand.m_words[1u];
      return *this;
   }

   constexpr auto
   operator|=(pcg_simd_state operand) -> pcg_simd_state& {
      m_words[0u] |= operand.m_words[0u];
      m_words[1u] |= operand.m_words[1u];
      return *this;
   }

   constexpr auto
   operator^=(pcg_simd_state operand) -> pcg_simd_state& {
      m_words[0u] ^= operand.m_words[0u];
      m_words[1u] ^= operand.m_words[1u];
      return *this;
   }

   constexpr auto
   operator>>=(idx count) -> pcg_simd_state& {
      m_words[0u] >>= uint8(count.raw);
      m_words[1u] >>= uint8(count.raw);
      return *this;
   }

   constexpr auto
   operator<<=(idx count) -> pcg_simd_state& {
      m_words[0u] <<= uint8(count.raw);
      m_words[1u] <<= uint8(count.raw);
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
      left.m_words[0u] -= right.m_words[0u];
      left.m_words[1u] -= right.m_words[1u];
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
      value >>= count;
      return value;
   }

   [[nodiscard]]
   friend constexpr auto
   operator<<(pcg_simd_state value, idx count) -> pcg_simd_state {
      value <<= count;
      return value;
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(pcg_simd_state const& left, pcg_simd_state const& right) -> bool {
      for (idx lane = 0u; lane < word_type::abi_type::lanes; ++lane) {
         if (left.lane(lane) != right.lane(lane)) {
            return false;
         }
      }
      return true;
   }
};

// PCG64 state cannot be stored in singular SIMD lanes.
template <is_simd Word>
class pcg_simd_state<Word, false> {
 public:
   using word_type = Word;
   using lane_type = word_type::value_type;
   using lane_raw = raw_arithmetic_type<lane_type>;
   using scalar_type =
      conditional<sizeof(lane_type) == 4u, uint8, unsigned __int128>;
   using mask_type = word_type::mask_type;

 private:
   static constexpr idx word_bytes = sizeof(lane_type);
   word_type m_low = 0u;
   word_type m_high = 0u;

 public:
   constexpr pcg_simd_state() = default;

   constexpr pcg_simd_state(scalar_type value)
       : m_low(lane_type(lane_raw(make_raw_arithmetic(value)))),
         m_high(lane_type(
            lane_raw(make_raw_arithmetic(value >> (word_bytes * 8u).raw))
         )) {
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
               + lemire_multiply_high(m_low, operand.m_low);
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
      if (count < word_bytes * 8u) {
         auto const shift = lane_type(count.raw);
         auto const inverse = lane_type((word_bytes * 8u).raw - count.raw);
         return {
            (value.m_low >> shift) | (value.m_high << inverse),
            value.m_high >> shift,
         };
      }
      if (count < word_bytes * 16u) {
         return {
            value.m_high >> lane_type(count.raw - (word_bytes * 8u).raw),
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
      if (count < word_bytes * 8u) {
         auto const shift = lane_type(count.raw);
         auto const inverse = lane_type((word_bytes * 8u).raw - count.raw);
         return {
            value.m_low << shift,
            (value.m_high << shift) | (value.m_low >> inverse),
         };
      }
      if (count < word_bytes * 16u) {
         return {
            word_type(0u),
            value.m_low << lane_type(count.raw - (word_bytes * 8u).raw),
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

template <typename State>
inline constexpr idx pcg_state_bytes = sizeof(State);

template <typename T>
   requires(!is_simd<T> && (sizeof(T) == 4u || sizeof(T) == 8u))
struct pcg_types<T> {
   using result = uint_fixed<sizeof(T)>;
   // PCG state is always wider than the result value.
   using state = conditional<sizeof(T) == 4u, uint8, unsigned __int128>;
};

template <typename Lane, typename Abi>
   requires(sizeof(Lane) == 4u || sizeof(Lane) == 8u)
struct pcg_types<simd<Lane, Abi>> {
   using result_lane = uint_fixed<sizeof(Lane)>;
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
   requires(!is_simd<Result>)
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

template <is_simd Result, typename State, pcg_permutation permutation>
[[nodiscard]]
constexpr auto
pcg_output(State state) -> Result {
   using lane_type = Result::value_type;
   using lane_raw = raw_arithmetic_type<lane_type>;
   constexpr idx word_bytes = sizeof(lane_type);
   constexpr bool equal_width_state = is_same<State, Result>;

   auto const rotate = [=](Result value, Result count) {
      constexpr auto mask = lane_type(word_bytes * 8u - 1u);
      return (value >> count) | (value << ((Result(0u) - count) & mask));
   };

   if constexpr (permutation == pcg_permutation::xorshift_high_random_rotate) {
      if constexpr (requires { state.template output<permutation>(); }) {
         return state.template output<permutation>();
      } else {
         Result const word = (((state >> 18u) ^ state) >> 27u).low();
         return rotate(word, state.high() >> 27u);
      }
   }

   if constexpr (permutation == pcg_permutation::xorshift_high_random_shift) {
      if constexpr (requires { state.template output<permutation>(); }) {
         return state.template output<permutation>();
      } else {
         State const mixed = (state >> 22u) ^ state;
         Result const shift = (state.high() >> 29u) + 22u;
         return (mixed.low() >> shift)
                | (mixed.high() << (Result(lane_type(32u)) - shift));
      }
   }

   if constexpr (permutation == pcg_permutation::xorshift_low_random_rotate) {
      return rotate(state.high() ^ state.low(), state.high() >> 58u);
   }

   if constexpr (
      permutation == pcg_permutation::random_xorshift_multiply_xorshift
   ) {
      static_assert(equal_width_state);
      Result word = ((state >> ((state >> 59u) + 5u)) ^ state)
                    * lane_type(12'605'985'483'714'917'081ull);
      return word ^ (word >> 43u);
   }

   if constexpr (permutation == pcg_permutation::double_xorshift_multiply) {
      if constexpr (requires { state.template output<permutation>(); }) {
         return state.template output<permutation>();
      } else {
         Result word = state.high();
         if constexpr (word_bytes == 4u) {
            word ^= word >> 16u;
            word *= lane_type(lane_raw(make_raw_arithmetic(
               pcg_constants<typename State::scalar_type>::cheap_multiplier
            )));
            word ^= word >> 24u;
         } else {
            word ^= word >> 32u;
            word *= lane_type(lane_raw(make_raw_arithmetic(
               pcg_constants<typename State::scalar_type>::cheap_multiplier
            )));
            word ^= word >> 48u;
         }
         return word * (state.low() | 1u);
      }
   }
}

template <is_simd Result, typename State>
[[nodiscard]]
constexpr auto
pcg_pack_states(array<State, Result::abi_type::lanes> const& states) {
   if constexpr (sizeof(State) == sizeof(typename Result::value_type)) {
      Result result;
      for (idx lane = 0u; lane < Result::abi_type::lanes; ++lane) {
         result.set_lane(lane, typename Result::value_type(states[lane]));
      }
      return result;
   } else {
      using packed_state = pcg_simd_state<Result>;
      using lane_type = Result::value_type;
      using lane_raw = raw_arithmetic_type<lane_type>;
      constexpr idx word_bytes = sizeof(lane_type);
      Result low;
      Result high;
      for (idx lane = 0u; lane < Result::abi_type::lanes; ++lane) {
         auto const raw_state = make_raw_arithmetic(states[lane]);
         low.set_lane(lane, lane_type(lane_raw(raw_state)));
         high.set_lane(
            lane, lane_type(lane_raw(raw_state >> (word_bytes * 8u).raw))
         );
      }
      return packed_state(low, high);
   }
}

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
   static constexpr bool enable_batch_fill = false;
   // Packed 16-byte PCG64 recurrence and unsigned high-half 8-byte multiply are
   // inefficient on SSE/AVX, so only vectorize PCG32.
   static constexpr bool enable_exact_bulk_fill =
      sizeof(State) == 8u && sizeof(result_type) == 4u;

 private:
   using state_type = State;

   state_type m_state = detail::pcg_initial_state<state_type, stream>;
   state_type m_increment = detail::pcg_initial_increment<state_type, stream>;

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
   operator()(result_type minimum, result_type maximum) -> result_type {
      return minimum + (*this)(maximum - minimum + 1u);
   }

 private:
   friend class detail::random_batch_access;

   template <typename Abi, idx chain_count>
   class exact_bulk_session {
      static_assert(
         chain_count == 1u || chain_count == 2u || chain_count == 4u
      );

      using batch_type = simd<result_type, Abi>;
      using batch_state = detail::pcg_simd_state<batch_type>;
      static constexpr idx lanes = batch_type::abi_type::lanes;

      array<batch_state, chain_count> m_states;
      batch_state m_multiplier;
      batch_state m_increment;

    public:
      constexpr explicit exact_bulk_session(pcg_engine const& generator) {
         state_type state = generator.m_state;
         state_type const multiplier_value = generator.multiplier();
         state_type const increment_value = generator.increment();
         if constexpr (!output_previous) {
            state = state * multiplier_value + increment_value;
         }

         for (idx chain = 0u; chain < chain_count; ++chain) {
            array<state_type, lanes> states;
            for (idx lane = 0u; lane < lanes; ++lane) {
               states[lane] = state;
               state = state * multiplier_value + increment_value;
            }
            m_states[chain] = detail::pcg_pack_states<batch_type>(states);
         }

         state_type multiplier_coefficient = 1u;
         state_type increment_coefficient = 0u;
         for (idx index = 0u; index < lanes * chain_count; ++index) {
            increment_coefficient =
               increment_coefficient * multiplier_value + 1u;
            multiplier_coefficient *= multiplier_value;
         }
         m_multiplier = batch_state(multiplier_coefficient);
         m_increment = batch_state(increment_coefficient * increment_value);
      }

      template <idx chain>
      [[nodiscard]]
      constexpr auto
      generate() -> batch_type {
         static_assert(chain < chain_count);
         batch_state const old_state = m_states[chain];
         m_states[chain] = old_state * m_multiplier + m_increment;
         return detail::pcg_output<batch_type, batch_state, permutation>(
            old_state
         );
      }
   };

   template <typename Abi, idx chain_count>
   [[nodiscard]]
   constexpr auto
   make_exact_bulk_session() const
      requires(sizeof(state_type) == sizeof(result_type) * 2u)
   {
      return exact_bulk_session<Abi, chain_count>(*this);
   }

   template <typename Abi>
   [[nodiscard]]
   constexpr auto
   generate_exact_batch() -> simd<result_type, Abi>
      requires(!is_simd<T>)
   {
      using batch_type = simd<result_type, Abi>;
      using batch_state = conditional<
         sizeof(state_type) == sizeof(result_type), batch_type,
         detail::pcg_simd_state<batch_type>>;
      constexpr idx lanes = batch_type::abi_type::lanes;
      array<state_type, lanes> multiplier_coefficients;
      array<state_type, lanes> increment_coefficients;
      state_type multiplier_coefficient = 1u;
      state_type increment_coefficient = 0u;

      for (idx lane = 0u; lane < lanes; ++lane) {
         if constexpr (!output_previous) {
            increment_coefficient = increment_coefficient * multiplier() + 1u;
            multiplier_coefficient *= multiplier();
         }
         multiplier_coefficients[lane] = multiplier_coefficient;
         increment_coefficients[lane] = increment_coefficient;
         if constexpr (output_previous) {
            increment_coefficient = increment_coefficient * multiplier() + 1u;
            multiplier_coefficient *= multiplier();
         }
      }

      batch_state const states =
         detail::pcg_pack_states<batch_type>(multiplier_coefficients)
            * batch_state(m_state)
         + detail::pcg_pack_states<batch_type>(increment_coefficients)
              * batch_state(increment());
      m_state =
         multiplier_coefficient * m_state + increment_coefficient * increment();
      return detail::pcg_output<batch_type, batch_state, permutation>(states);
   }

 public:
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

 private:
   constexpr void
   advance(state_type count) {
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

 public:
   // Skip this many outputs.
   constexpr void
   discard(uint8 count) {
      advance(static_cast<state_type>(count));
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(pcg_engine const& left, pcg_engine const& right) -> bool {
      return left.m_state == right.m_state
             && left.increment() == right.increment();
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

 private:
   using state_type = State;
   using lane_type = result_type::value_type;
   using lane_raw = raw_arithmetic_type<lane_type>;
   using expected_state = detail::pcg_simd_state<result_type>;
   using mask_type = result_type::mask_type;
   static constexpr bool equal_width_state = is_same<state_type, result_type>;
   using scalar_state = conditional<
      equal_width_state, lane_type, typename expected_state::scalar_type>;
   static constexpr idx word_bytes = sizeof(lane_type);
   static constexpr idx state_bytes = detail::pcg_state_bytes<scalar_state>;

   static_assert(
      (is_same<state_type, expected_state>
       && permutation != pcg_permutation::random_xorshift_multiply_xorshift)
      || (equal_width_state && word_bytes == 8u && permutation == pcg_permutation::random_xorshift_multiply_xorshift)
   );

   state_type m_state =
      state_type(detail::pcg_initial_state<scalar_state, stream>);
   state_type m_increment =
      state_type(detail::pcg_initial_increment<scalar_state, stream>);
   result_type m_lane_offsets = simd_iota<result_type>(0u) * 2u;

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
      return detail::pcg_output<result_type, state_type, permutation>(state);
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
   constexpr auto
   operator()(result_type minimum, result_type maximum) -> result_type {
      return minimum + (*this)(maximum - minimum + 1u);
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

 private:
   constexpr void
   advance(state_type count) {
      state_type accumulated_multiplier = 1u;
      state_type accumulated_increment = 0u;
      state_type current_multiplier = multiplier();
      state_type current_increment = increment();

      for (idx index = 0u; index < state_bytes * 8u; ++index) {
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

 public:
   constexpr void
   discard(result_type count) {
      advance(state_type(count));
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(pcg_engine const& left, pcg_engine const& right) -> bool {
      return left.m_state == right.m_state
             && left.increment() == right.increment();
   }
};

}  // namespace detail

template <typename T, pcg_stream stream = pcg_stream::setseq>
using pcg_engine = detail::pcg_engine<T, stream>;

template <typename T, pcg_stream stream = pcg_stream::setseq>
   requires(
      sizeof(detail::random_scalar<T>) == 4u
      || sizeof(detail::random_scalar<T>) == 8u
   )
class pcg_dxsm_engine : public detail::pcg_engine<
                           T, stream, typename detail::pcg_types<T>::state,
                           detail::pcg_permutation::double_xorshift_multiply,
                           true, sizeof(detail::random_scalar<T>) == 8u> {
   using base = detail::pcg_engine<
      T, stream, typename detail::pcg_types<T>::state,
      detail::pcg_permutation::double_xorshift_multiply, true,
      sizeof(detail::random_scalar<T>) == 8u>;

 public:
   using base::base;
};

}  // namespace cat
