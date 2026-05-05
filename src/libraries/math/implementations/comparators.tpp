// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <typename T = void>
struct equal_to;

template <typename T>
struct equal_to {
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T const& left, T const& right) const -> bool {
      return left == right;
   }
};

template <>
struct equal_to<void> {
   using is_transparent = void;

   template <typename T, typename U>
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T&& left, U&& right) const
      -> decltype($fwd(left) == $fwd(right)) {
      return $fwd(left) == $fwd(right);
   }
};

template <typename T = void>
struct not_equal_to;

template <typename T>
struct not_equal_to {
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T const& left, T const& right) const -> bool {
      return left != right;
   }
};

template <>
struct not_equal_to<void> {
   using is_transparent = void;

   template <typename T, typename U>
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T&& left, U&& right) const
      -> decltype($fwd(left) != $fwd(right)) {
      return $fwd(left) != $fwd(right);
   }
};

template <typename T = void>
struct less;

template <typename T>
struct less {
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T const& left, T const& right) const -> bool {
      return left < right;
   }
};

template <>
struct less<void> {
   using is_transparent = void;

   template <typename T, typename U>
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T&& left, U&& right) const -> decltype($fwd(left) < $fwd(right)) {
      return $fwd(left) < $fwd(right);
   }
};

template <typename T = void>
struct greater;

template <typename T>
struct greater {
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T const& left, T const& right) const -> bool {
      return left > right;
   }
};

template <>
struct greater<void> {
   using is_transparent = void;

   template <typename T, typename U>
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T&& left, U&& right) const -> decltype($fwd(left) > $fwd(right)) {
      return $fwd(left) > $fwd(right);
   }
};

template <typename T = void>
struct less_equal;

template <typename T>
struct less_equal {
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T const& left, T const& right) const -> bool {
      return left <= right;
   }
};

template <>
struct less_equal<void> {
   using is_transparent = void;

   template <typename T, typename U>
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T&& left, U&& right) const
      -> decltype($fwd(left) <= $fwd(right)) {
      return $fwd(left) <= $fwd(right);
   }
};

template <typename T = void>
struct greater_equal;

template <typename T>
struct greater_equal {
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T const& left, T const& right) const -> bool {
      return left >= right;
   }
};

template <>
struct greater_equal<void> {
   using is_transparent = void;

   template <typename T, typename U>
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T&& left, U&& right) const
      -> decltype($fwd(left) >= $fwd(right)) {
      return $fwd(left) >= $fwd(right);
   }
};

template <typename T = void>
struct compare_three_way;

template <typename T>
struct compare_three_way {
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T const& left, T const& right) const {
      return left <=> right;
   }
};

template <>
struct compare_three_way<void> {
   using is_transparent = void;

   template <typename T, typename U>
   [[nodiscard, gnu::always_inline]]
   constexpr auto
   operator()(T&& left, U&& right) const
      -> decltype($fwd(left) <=> $fwd(right)) {
      return $fwd(left) <=> $fwd(right);
   }
};

}  // namespace cat
