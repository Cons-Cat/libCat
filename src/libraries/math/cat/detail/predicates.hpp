// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

namespace detail {

struct is_even_impl {
   template <is_integral T>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   operator()(T value) -> bool {
      return (make_raw_arithmetic(value) & 1u) == 0;
   }
};

struct is_odd_impl {
   template <is_integral T>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   operator()(T value) -> bool {
      return (make_raw_arithmetic(value) & 1u) != 0;
   }
};

// Tag types for the binary comparator op. `apply` is the binary operator,
// named so the shared template can dispatch by tag.
struct equal_to_op {
   template <typename Left, typename Right>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   apply(Left&& left, Right&& right) -> decltype($fwd(left) == $fwd(right)) {
      return $fwd(left) == $fwd(right);
   }
};

struct not_equal_to_op {
   template <typename Left, typename Right>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   apply(Left&& left, Right&& right) -> decltype($fwd(left) != $fwd(right)) {
      return $fwd(left) != $fwd(right);
   }
};

struct less_op {
   template <typename Left, typename Right>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   apply(Left&& left, Right&& right) -> decltype($fwd(left) < $fwd(right)) {
      return $fwd(left) < $fwd(right);
   }
};

struct greater_op {
   template <typename Left, typename Right>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   apply(Left&& left, Right&& right) -> decltype($fwd(left) > $fwd(right)) {
      return $fwd(left) > $fwd(right);
   }
};

struct less_equal_op {
   template <typename Left, typename Right>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   apply(Left&& left, Right&& right) -> decltype($fwd(left) <= $fwd(right)) {
      return $fwd(left) <= $fwd(right);
   }
};

struct greater_equal_op {
   template <typename Left, typename Right>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   apply(Left&& left, Right&& right) -> decltype($fwd(left) >= $fwd(right)) {
      return $fwd(left) >= $fwd(right);
   }
};

struct compare_three_way_op {
   template <typename Left, typename Right>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   apply(Left&& left, Right&& right) -> decltype($fwd(left) <=> $fwd(right)) {
      return $fwd(left) <=> $fwd(right);
   }
};

// Curry-able binary comparator. The type-level `Bound...` pack carries the
// bound arguments. For arity 2 the only valid bound counts are 0 and 1, so
// the template has two specializations (no bound, one bound). The pack
// shape is preserved so the same skeleton can be lifted to higher arities
// later.
template <typename Op, typename... Bound>
struct binary_comparator;

template <typename Op>
struct binary_comparator<Op> {
   using is_transparent = void;

   template <typename L, typename R>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(L&& l, R&& r) const {
      return Op::apply($fwd(l), $fwd(r));
   }

   template <typename T>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(T value) const -> binary_comparator<Op, T> {
      return {value};
   }
};

template <typename Op, typename Bound>
struct binary_comparator<Op, Bound> {
   using is_transparent = void;

   Bound bound;

   template <typename T>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(T&& t) const {
      return Op::apply(bound, $fwd(t));
   }
};

// Curry-able `is_divisible_by`. Mirrors the binary-comparator pattern: the
// type-level `Bound...` pack carries the bound arguments and only 0 or 1
// are valid for this arity-2 op.
//
// Currying binds the dividend, so `is_divisible_by(12)(n)` reads as
// "is `12` divisible by `n`" and asks whether `n` is a divisor of `12`.
//
// TODO: same generalization as `binary_comparator` (above). The currying
// form here also loses the `enable_if`-constrained fast paths the original
// overload set had (skipping the `divisor == 0` and `divisor == -1`
// short-circuits when the compiler could prove `divisor > 0`). Restore
// those once a real bind machinery lands.
template <typename... Bound>
struct is_divisible_by_impl;

template <typename T, typename U>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
is_divisible_by_apply(T value, U divisor) -> bool {
   if (divisor == U(0)) {
      return value == T(0);
   }
   if constexpr (is_signed<U>) {
      if (divisor == U(-1)) {
         return true;
      }
   }
   return (value % divisor) == T(0);
}

template <>
struct is_divisible_by_impl<> {
   template <is_integral T, is_implicitly_convertible<T> U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(T value, U divisor) const -> bool {
      return is_divisible_by_apply(value, divisor);
   }

   template <is_integral T>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(T value) const -> is_divisible_by_impl<T> {
      return {value};
   }
};

template <typename Bound>
struct is_divisible_by_impl<Bound> {
   Bound bound;

   template <is_implicitly_convertible<Bound> U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(U divisor) const -> bool {
      return is_divisible_by_apply(bound, divisor);
   }
};

}  // namespace detail

inline constexpr detail::is_even_impl is_even{};
inline constexpr detail::is_odd_impl is_odd{};
inline constexpr detail::is_divisible_by_impl<> is_divisible_by{};

inline constexpr detail::binary_comparator<detail::equal_to_op> is_equal_to{};
inline constexpr detail::binary_comparator<detail::not_equal_to_op>
   is_not_equal_to{};
inline constexpr detail::binary_comparator<detail::less_op> is_less{};
inline constexpr detail::binary_comparator<detail::greater_op> is_greater{};
inline constexpr detail::binary_comparator<detail::less_equal_op>
   is_less_equal{};
inline constexpr detail::binary_comparator<detail::greater_equal_op>
   is_greater_equal{};
inline constexpr detail::binary_comparator<detail::compare_three_way_op>
   compare_three_way{};

}  // namespace cat
