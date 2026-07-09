// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
// Drain into a default-constructed container, preferring the container's
// fast `append_range` and falling back to element-wise `push_back`.
template <typename Container>
struct to_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, to_impl /*to_impl*/) -> Container {
      Container result{};
      auto&& range = unwrap_ref(incoming);

      if constexpr (requires { result.append_range($fwd(range)).assert(); }) {
         // This breaks the iterable model internally, but it is an essential
         // optimization.
         result.append_range($fwd(range)).assert();
      } else {
         auto context = iterate(incoming);
         context.run_while([&result](auto&& element) -> bool {
            result.push_back($fwd(element));
            return true;
         });
      }

      return result;
   }
};
}  // namespace detail

// Drain into a default-constructed container using `push_back`. This is a
// terminal algorithm.
template <typename Container>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
to() -> detail::to_impl<Container> {
   return {};
}

}  // namespace cat
