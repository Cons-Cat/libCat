// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {

// `sum(source)` reduces with `+`.
template <is_iterable Iterable>
constexpr auto
sum(Iterable&& source) -> iterable_value_type<Iterable> {
   iterable_value_type<Iterable> total{};
   auto context = iterate(source);

   context.run_while([&total](auto&& element) -> bool {
      total = total + fwd(element);
      return true;
   });

   return total;
}

namespace detail {
struct sum_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, sum_impl /*sum_impl*/) {
      return sum(fwd(incoming));
   }
};
}  // namespace detail

// Reduce the incoming elements with `+`. This is a terminal algorithm.
constexpr auto
sum() -> detail::sum_impl {
   return {};
}

}  // namespace cat
