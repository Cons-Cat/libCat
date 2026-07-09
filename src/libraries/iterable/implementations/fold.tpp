// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Callback, typename Init>
struct fold_impl {
   Callback callback;
   Init init;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, fold_impl self) -> Init {
      auto context = iterate(incoming);
      context.run_while([&self](auto&& element) -> bool {
         self.init = self.callback(move(self.init), $fwd(element));
         return true;
      });
      return move(self.init);
   }
};
}  // namespace detail

// Left-fold with a callback and initial value. This is a terminal algorithm.
template <typename Callback, typename Init>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
fold(Callback callback, Init init) -> detail::fold_impl<Callback, Init> {
   return detail::fold_impl<Callback, Init>{move(callback), move(init)};
}

}  // namespace cat
