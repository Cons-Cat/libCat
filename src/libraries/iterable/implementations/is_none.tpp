// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Predicate>
struct is_none_impl {
   Predicate predicate;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, is_none_impl self) -> bool {
      return iterate(incoming).run_while([&self](auto&& element) -> bool {
         return !bool(self.predicate($fwd(element)));
      }) == iteration_result::complete;
   }
};
}  // namespace detail

// Evaluate true if `predicate` holds for no incoming element, or if there are
// none. Without a `predicate`, test each element's truthiness. This is a
// terminal algorithm.
template <typename Predicate = decltype([](auto const& element) -> bool {
             return bool(element);
          })>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
is_none(Predicate predicate = {}) -> detail::is_none_impl<Predicate> {
   return detail::is_none_impl<Predicate>{move(predicate)};
}

}  // namespace cat
