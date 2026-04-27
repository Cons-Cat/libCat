// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {

// Get the number of elements in an iterable.
template <is_iterable Iterable>
constexpr auto
count(Iterable&& source) -> idx {
   idx element_count{};
   auto context = iterate(source);

   context.run_while([&element_count](auto&&) -> bool {
      ++element_count;
      return true;
   });

   return element_count;
}

namespace detail {
struct count_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, count_impl /*count_impl*/) {
      return count(fwd(incoming));
   }
};
}  // namespace detail

// Count the incoming elements. This is a terminal algorithm.
constexpr auto
count() -> detail::count_impl {
   return {};
}

}  // namespace cat
