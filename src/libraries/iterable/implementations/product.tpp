// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
struct product_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, product_impl /*product_impl*/) {
      iterable_value_type<Iterable> total = 1;
      auto context = iterate(incoming);

      context.run_while([&total](auto&& element) -> bool {
         total = total * $fwd(element);
         return true;
      });

      return total;
   }
};
}  // namespace detail

// Reduce the incoming elements with *. This is a terminal algorithm.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
product() -> detail::product_impl {
   return {};
}

}  // namespace cat
