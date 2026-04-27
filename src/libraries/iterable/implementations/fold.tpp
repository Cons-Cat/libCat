// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {

// Left fold. Default accumulator is value-initialised
// `iterable_value_type<Iterable>`.
template <is_iterable Iterable, typename Callback,
          typename Init = iterable_value_type<Iterable>>
constexpr auto
fold(Iterable&& source, Callback callback, Init init = Init{}) -> Init {
   auto context = iterate(source);
   context.run_while([&init, &callback](auto&& element) -> bool {
      init = callback(move(init), fwd(element));
      return true;
   });
   return init;
}

}  // namespace cat

namespace cat::detail {
template <typename Callback, typename Init>
struct fold_impl {
   Callback callback;
   Init init;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, fold_impl self) -> Init {
      return fold(fwd(incoming), move(self.callback), move(self.init));
   }
};
}  // namespace cat::detail

namespace cat {

// Left-fold with a callback and initial value. This is a terminal algorithm.
template <typename Callback, typename Init>
constexpr auto
fold(Callback callback, Init init) -> detail::fold_impl<Callback, Init> {
   return detail::fold_impl<Callback, Init>{move(callback), move(init)};
}

}  // namespace cat
