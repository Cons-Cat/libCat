// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Predicate>
struct is_any_impl {
   Predicate predicate;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, is_any_impl self) -> bool {
      return iterate(incoming).run_while([&self](auto&& element) -> bool {
         return !bool(self.predicate($fwd(element)));
      }) != iteration_result::complete;
   }
};
}  // namespace detail

// Evaluate true if `predicate` holds for at least one incoming element. Without
// a `predicate`, test each element's truthiness. This is a terminal algorithm.
template <typename Predicate = decltype([](auto const& element) -> bool {
             return bool(element);
          })>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
is_any(Predicate predicate = {}) -> detail::is_any_impl<Predicate> {
   return detail::is_any_impl<Predicate>{move(predicate)};
}

}  // namespace cat
