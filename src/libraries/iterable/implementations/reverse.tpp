// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
template <typename Base>
struct reverse_view_impl : iterable_interface {
   Base m_base;

   constexpr auto
   iterate() -> decltype(::cat::reverse_iterate(detail::unwrap_ref(m_base))) {
      return ::cat::reverse_iterate(detail::unwrap_ref(m_base));
   }

   constexpr auto
   iterate() const
      -> decltype(::cat::reverse_iterate(detail::unwrap_ref(m_base)))
      requires(is_reverse_iterable<decltype(detail::unwrap_ref(m_base))>)
   {
      return ::cat::reverse_iterate(detail::unwrap_ref(m_base));
   }
};

template <typename Base>
using reverse_view = reverse_view_impl<Base>;

struct reverse_impl : view_interface<reverse_impl> {
   template <is_borrow_acceptable Iterable>
      requires(is_reverse_iterable<Iterable>)
   constexpr auto
   apply(Iterable&& incoming) const -> reverse_view<Iterable> {
      return reverse_view<Iterable>{{}, fwd(incoming)};
   }
};
}  // namespace detail

// `reverse()` returns a closure consumable by `data | reverse()`. The
// underlying iterable must model `is_reverse_iterable`.
constexpr auto
reverse() -> detail::reverse_impl {
   return detail::reverse_impl{{}};
}

}  // namespace cat
