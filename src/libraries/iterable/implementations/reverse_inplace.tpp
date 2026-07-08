// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/algorithm>
#include <cat/iterable>

namespace cat {
namespace detail {
template <typename T>
using reverse_inplace_base = remove_cvref<T>;

template <typename T>
concept has_void_reverse_inplace_member = requires {
   static_cast<void (reverse_inplace_base<T>::*)()>(
      &reverse_inplace_base<T>::reverse_inplace
   );
};

template <typename T>
concept has_const_self_reverse_inplace_member = requires {
   static_cast<reverse_inplace_base<T> (reverse_inplace_base<T>::*)() const>(
      &reverse_inplace_base<T>::reverse_inplace
   );
};

struct reverse_inplace_impl {
   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, reverse_inplace_impl /*reverse_inplace_impl*/)
      -> decltype(auto) {
      auto&& target = unwrap_ref(incoming);
      if constexpr (has_void_reverse_inplace_member<decltype(target)>) {
         target.reverse_inplace();
      } else if constexpr (
         has_const_self_reverse_inplace_member<decltype(target)>
      ) {
         static_cast<void>(target.reverse_inplace());
      } else {
         stepanov_reverse_inplace(target.begin(), target.end());
      }
      return $fwd(incoming);
   }
};
}  // namespace detail

// Reverse the incoming container in-place. This is a terminal algorithm.
constexpr auto
reverse_inplace() -> detail::reverse_inplace_impl {
   return {};
}

}  // namespace cat
