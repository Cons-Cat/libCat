// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
struct sum_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, sum_impl /*sum_impl*/) {
      iterable_value_type<Iterable> total{};
      auto context = iterate(incoming);

      context.run_while([&total](auto&& element) -> bool {
         total = total + $fwd(element);
         return true;
      });

      return total;
   }
};
}  // namespace detail

// Reduce the incoming elements with +. This is a terminal algorithm.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
sum() -> detail::sum_impl {
   return {};
}

}  // namespace cat
