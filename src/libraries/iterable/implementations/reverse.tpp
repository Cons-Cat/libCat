// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base>
struct reverse_view_impl : iterable_interface<> {
   Base m_base;

   // This works around limitation in recursive `concept`s.
   static constexpr bool is_borrowing_view = !is_view_owned<Base>;

   constexpr auto
   iterate(this auto&& self [[clang::lifetimebound]])
      -> decltype(reverse_iterate(unwrap_ref(self.m_base)))
      requires(
         is_view_owned<Base>
         && is_reverse_iterable<decltype(unwrap_ref(self.m_base))>
      )
   {
      return reverse_iterate(unwrap_ref(self.m_base));
   }

   constexpr auto
   iterate(this reverse_view_impl self [[clang::lifetimebound]])
      -> decltype(reverse_iterate(unwrap_ref(self.m_base)))
      requires(!is_view_owned<Base>)
   {
      return reverse_iterate(unwrap_ref(self.m_base));
   }
};

template <typename Base>
using reverse_view = reverse_view_impl<Base>;

struct reverse_impl : view_interface<reverse_impl> {
   template <is_borrow_acceptable Iterable>
      requires(is_reverse_iterable<Iterable>)
   constexpr auto
   apply(Iterable&& incoming) const -> reverse_view<Iterable> {
      return reverse_view<Iterable>{{}, $fwd(incoming)};
   }
};
}  // namespace detail

// `reverse()` returns a closure consumable by `data | reverse()`. The
// underlying iterable must model `is_reverse_iterable`. This is a view adaptor.
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
reverse() -> detail::reverse_impl {
   return detail::reverse_impl{{}};
}

}  // namespace cat
