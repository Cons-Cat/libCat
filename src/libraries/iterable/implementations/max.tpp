// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
struct max_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, max_impl /*max_impl*/) {
      using result_type = iterable_value_type<Iterable>;
      auto context = iterate(incoming);
      auto maybe_maximum = next_element(context);
      if (!maybe_maximum.has_value()) {
         return result_type{};
      }

      result_type maximum = maybe_maximum.value();
      context.run_while([&maximum](auto&& element) -> bool {
         maximum = element > maximum ? $fwd(element) : maximum;
         return true;
      });

      return maximum;
   }
};
}  // namespace detail

// Reduce the incoming elements to the largest. This is a terminal algorithm.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
max() -> detail::max_impl {
   return {};
}

}  // namespace cat
