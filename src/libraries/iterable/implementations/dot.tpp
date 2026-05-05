// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Other>
struct dot_impl {
   Other other;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, dot_impl self) {
      using result_type =
         remove_cvref<decltype(declval<iterable_element_type<Iterable>>()
                               * declval<iterable_element_type<Other>>())>;

      result_type total{};
      auto left_context = iterate(incoming);
      auto right_context = iterate(self.other);

      left_context.run_while(
         [&total, &right_context](auto&& left_element) -> bool {
            auto maybe_right_element = next_element(right_context);
            if (!maybe_right_element.has_value()) {
               return false;
            }
            total = total + ($fwd(left_element) * maybe_right_element.value());
            return true;
         });

      return total;
   }
};
}  // namespace detail

// Reduce the lockstep elementwise product of two iterables with +. Stops when
// either input is exhausted. This is a terminal algorithm.
template <typename Other>
constexpr auto
dot(Other&& other) -> detail::dot_impl<remove_cvref<Other>> {
   return detail::dot_impl<remove_cvref<Other>>{$fwd(other)};
}

}  // namespace cat
