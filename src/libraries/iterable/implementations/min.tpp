// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
struct min_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, min_impl /*min_impl*/) {
      using result_type = iterable_value_type<Iterable>;
      auto context = iterate(incoming);
      auto maybe_minimum = next_element(context);
      if (!maybe_minimum.has_value()) {
         return result_type{};
      }

      result_type minimum = maybe_minimum.value();
      context.run_while([&minimum](auto&& element) -> bool {
         minimum = element < minimum ? $fwd(element) : minimum;
         return true;
      });

      return minimum;
   }
};
}  // namespace detail

// Reduce the incoming elements to the smallest. This is a terminal algorithm.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
min() -> detail::min_impl {
   return {};
}

}  // namespace cat
