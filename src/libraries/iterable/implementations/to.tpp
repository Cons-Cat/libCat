// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat::detail {
// Drain into a default-constructed container using `push_back`. Composing this
// with `as_rvalue` is a basic idiom to move the values out from a pipeline into
// a container.
template <typename Container>
struct to_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, to_impl /*to_impl*/) -> Container {
      Container result{};
      auto context = iterate(incoming);

      // TODO: Optimize with a multi `push_back()`.
      context.run_while([&result](auto&& element) -> bool {
         result.push_back(fwd(element));
         return true;
      });

      return result;
   }
};
}  // namespace cat::detail

namespace cat {

// Drain into a default-constructed container using `push_back`. This is a
// terminal algorithm.
template <typename Container>
constexpr auto
to() -> detail::to_impl<Container> {
   return {};
}

}  // namespace cat
