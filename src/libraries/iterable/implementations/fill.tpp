// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>

namespace cat {
namespace detail {
// A real, non-deduced-this `fill` member. The pointer-to-member expression is
// ill-formed for the inherited `iterable_interface::fill` template, so this
// only matches containers with their own member `fill`.
template <typename T>
concept has_fill_member = requires { &remove_cvref<T>::fill; };

template <typename Value>
struct fill_impl {
   Value m_value;

   template <is_iterable Iterable>
      requires(!is_const<Iterable>)
   friend constexpr auto
   operator|(Iterable&& incoming, fill_impl self) -> decltype(auto) {
      auto&& range = unwrap_ref(incoming);
      if constexpr (has_fill_member<decltype(range)> && requires {
                                                           range.fill(
                                                              self.m_value
                                                           );
                                                        }) {
         range.fill(self.m_value);
      } else {
         auto context = iterate(range);
         context.run_while([&self](auto&& element) -> bool {
            element = self.m_value;
            return true;
         });
      }
      return $fwd(incoming);
   }
};
}  // namespace detail

// Assign `value` to every element of the incoming iterable and return it
// for further chaining.
template <typename Value>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
fill(Value value) -> detail::fill_impl<Value> {
   return {$fwd(value)};
}

}  // namespace cat
