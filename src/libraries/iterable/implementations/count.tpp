// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
struct count_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, count_impl /*count_impl*/) -> idx {
      if constexpr (requires { unwrap_ref(incoming).size(); }) {
         return unwrap_ref(incoming).size();
      }

      idx element_count{};
      auto context = iterate(incoming);

      context.run_while([&element_count](auto&&) -> bool {
         ++element_count;
         return true;
      });

      return element_count;
   }
};
}  // namespace detail

// Count the incoming elements. This is a terminal algorithm.
constexpr auto
count() -> detail::count_impl {
   return {};
}

}  // namespace cat
