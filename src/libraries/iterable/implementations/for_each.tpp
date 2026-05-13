// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Callback>
struct for_each_impl {
   Callback callback;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, for_each_impl self) -> Callback {
      auto context = iterate(incoming);

      context.run_while([&self](auto&& element) -> bool {
         self.callback($fwd(element));
         return true;
      });

      return $fwd(self).callback;
   }
};
}  // namespace detail

// Invoke a perfect-forwarded callback on every element. This is a terminal
// algorithm.
template <typename Callback>
constexpr auto
for_each(Callback&& callback) -> detail::for_each_impl<Callback> {
   return detail::for_each_impl<Callback>{$fwd(callback)};
}

}  // namespace cat
