// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Out-of-line definitions for `cat::container_interface` slice methods.
// These cannot live inside `<cat/container>` because `maybe_span<T>` requires
// `cat::span<T>` to be a complete type, and `<cat/span>` itself depends on
// `<cat/container>`. The cycle is broken by including this file at the bottom
// of `<cat/span>`, after `span<T>` and `maybe_span<T>` are defined.

#include <cat/container>
#include <cat/span>

namespace cat {

template <typename Derived, typename T>
constexpr auto
container_interface<Derived, T>::subspan(idx start_index, idx end_index) &
   requires(container_interface<Derived, T>::is_array_like)
{
   Derived& self = static_cast<Derived&>(*this);
   if (start_index > end_index || end_index > self.capacity()) {
      return maybe_span<T>(nullopt);
   }
   return maybe_span<T>(
      span<T>(self.data() + start_index, idx(end_index - start_index))
   );
}

template <typename Derived, typename T>
constexpr auto
container_interface<Derived, T>::subspan(idx start_index, idx end_index) const&
   requires(container_interface<Derived, T>::is_array_like)
{
   Derived const& self = static_cast<Derived const&>(*this);
   if (start_index > end_index || end_index > self.capacity()) {
      return maybe_span<T const>(nullopt);
   }
   return maybe_span<T const>(
      span<T const>(self.data() + start_index, idx(end_index - start_index))
   );
}

template <typename Derived, typename T>
constexpr auto
container_interface<Derived, T>::first(idx count) &
   requires(container_interface<Derived, T>::is_array_like)
{
   return this->subspan(0u, count);
}

template <typename Derived, typename T>
constexpr auto
container_interface<Derived, T>::first(idx count) const&
   requires(container_interface<Derived, T>::is_array_like)
{
   return this->subspan(0u, count);
}

template <typename Derived, typename T>
constexpr auto
container_interface<Derived, T>::last(idx count) &
   requires(container_interface<Derived, T>::is_array_like)
{
   Derived& self = static_cast<Derived&>(*this);
   if (count > self.size()) {
      return maybe_span<T>(nullopt);
   }
   return this->subspan(idx(self.size() - count), self.size());
}

template <typename Derived, typename T>
constexpr auto
container_interface<Derived, T>::last(idx count) const&
   requires(container_interface<Derived, T>::is_array_like)
{
   Derived const& self = static_cast<Derived const&>(*this);
   if (count > self.size()) {
      return maybe_span<T const>(nullopt);
   }
   return this->subspan(idx(self.size() - count), self.size());
}

}  // namespace cat
