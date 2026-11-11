// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>
#include <cat/math>

namespace cat {
namespace detail {

template <typename T>
concept has_iterable_extent = requires(remove_cvref<T>& value) {
                                 {
                                    value.size()
                                 } -> is_implicitly_convertible<idx>;
                              };

// Left fold `init = op1(init, op2(left, right))` across `incoming`, stepping
// the second iterable in lockstep. The first iterable decides how many steps
// run. A second iterable with a known length must be at least that long.
// Without a length, a short second iterable stops the fold the way `dot` does.
template <typename Other, typename Init, typename Op1, typename Op2>
struct inner_product_impl {
   Other other;
   Init init;
   Op1 op1;
   Op2 op2;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, inner_product_impl self) -> Init {
      if constexpr (
         has_iterable_extent<Iterable> && has_iterable_extent<Other>
      ) {
         assert(self.other.size() >= incoming.size());
      }

      auto left_context = iterate(incoming);
      auto right_context = iterate(self.other);
      left_context.run_while(
         [&self, &right_context](auto&& left_element) -> bool {
            auto maybe_right_element = next_element(right_context);
            if (maybe_right_element.is_empty()) {
               return false;
            }
            self.init = self.op1(
               move(self.init),
               self.op2($fwd(left_element), maybe_right_element.value())
            );
            return true;
         }
      );
      return move(self.init);
   }
};

}  // namespace detail

// `std::inner_product` over two iterables. Default operations are `*` and `+`,
// with no conjugation. This is a terminal algorithm.
template <typename Other, typename Init>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
inner_product(Other&& other, Init init) -> detail::inner_product_impl<
   remove_cvref<Other>, Init, detail::inner_product_add,
   detail::inner_product_multiply> {
   return {
      $fwd(other),
      move(init),
      detail::inner_product_add{},
      detail::inner_product_multiply{},
   };
}

// `op1` accumulates. `op2` combines one element from each iterable.
template <typename Other, typename Init, typename Op1, typename Op2>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
inner_product(Other&& other, Init init, Op1 op1, Op2 op2)
   -> detail::inner_product_impl<remove_cvref<Other>, Init, Op1, Op2> {
   return {$fwd(other), move(init), move(op1), move(op2)};
}

}  // namespace cat
