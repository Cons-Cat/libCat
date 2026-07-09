// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
struct popcount_impl {
   // Count set bits in an iterable.
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, popcount_impl /*popcount_impl*/) -> idx {
      if constexpr (requires { cat::popcount(incoming); }) {
         return cat::popcount(incoming);
      } else {
         idx count = 0u;
         auto context = iterate(incoming);
         context.run_while([&count](auto&& element) -> bool {
            if constexpr (requires { cat::popcount(element); }) {
               count += cat::popcount(element);
            } else if (element) {
               ++count;
            }
            return true;
         });
         return count;
      }
   }
};
}  // namespace detail

// Return an iterable terminal that counts set bits.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
popcount() -> detail::popcount_impl {
   return {};
}

}  // namespace cat
