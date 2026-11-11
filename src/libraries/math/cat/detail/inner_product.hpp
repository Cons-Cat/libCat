// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {
namespace detail {

// `std::inner_product`'s default operations, spelled with this library's `*`
// and `+` so a quantity and its overflow or precision policy survive the fold.
struct inner_product_multiply {
   template <typename T, typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(T&& left, U&& right) const -> decltype(auto) {
      return $fwd(left) * $fwd(right);
   }
};

struct inner_product_add {
   template <typename T, typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator()(T&& left, U&& right) const -> decltype(auto) {
      return $fwd(left) + $fwd(right);
   }
};

}  // namespace detail

// A scalar that `*` and `+` can fold. Real arithmetic, a raw `_Complex`, and a
// libCat complex all qualify. Nothing here conjugates: that is `dot`.
template <typename T>
concept is_inner_product_value = is_arithmetic<T> || is_complex<T>;

// Ordered left fold of pairs into `init`: `init = init + (value * other)`,
// then the same for each remaining pair. `init` decides the accumulator type.
template <
   typename Init, is_inner_product_value T, is_inner_product_value U,
   is_inner_product_value... Rest>
   requires(sizeof...(Rest) % 2 == 0)
[[nodiscard]]
constexpr auto
inner_product(Init init, T value, U other, Rest... rest) noexcept {
   init = detail::inner_product_add{}(
      move(init), detail::inner_product_multiply{}(value, other)
   );
   if constexpr (sizeof...(Rest) == 0) {
      return init;
   } else {
      return inner_product(move(init), rest...);
   }
}

// One scalar pair with caller-supplied operations. `op1` accumulates and `op2`
// combines the pair, the same order as `std::inner_product`.
template <
   typename Init, is_inner_product_value T, is_inner_product_value U,
   typename Op1, typename Op2>
   requires(
      is_invocable<Op2, T, U>
      && is_invocable<
         Op1, Init, decltype(declval<Op2&>()(declval<T>(), declval<U>()))>
   )
[[nodiscard]]
constexpr auto
inner_product(Init init, T value, U other, Op1 op1, Op2 op2) noexcept(
   is_nothrow_invocable<Op2, T, U>
   && is_nothrow_invocable<
      Op1, Init, decltype(declval<Op2&>()(declval<T>(), declval<U>()))>
) {
   return op1(move(init), op2(value, other));
}

}  // namespace cat
