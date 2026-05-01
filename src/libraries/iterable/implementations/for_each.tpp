// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {

// `for_each(source, callback)` invokes `callback` on every element of the
// iterable and returns `callback`.
template <is_iterable Iterable, typename Callback>
constexpr auto
for_each(Iterable&& source, Callback callback) -> Callback {
   auto context = iterate(source);

   context.run_while([&callback](auto&& element) -> bool {
      callback($fwd(element));
      return true;
   });

   return callback;
}

namespace detail {
template <typename Callback>
struct for_each_impl {
   Callback callback;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, for_each_impl self) -> Callback {
      return for_each($fwd(incoming), move(self.callback));
   }
};
}  // namespace detail

// Invoke a callback on every element. This is a terminal algorithm.
template <typename Callback>
constexpr auto
for_each(Callback callback) -> detail::for_each_impl<Callback> {
   return detail::for_each_impl<Callback>{move(callback)};
}

}  // namespace cat
