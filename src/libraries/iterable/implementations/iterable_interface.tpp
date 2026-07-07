// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/iterable_interface.hpp>

#include <cat/iterable>

// Out-of-line `iterable_interface` member templates. This file is included only
// at the end of `<cat/iterable>`, so every `cat::` entity named here is a
// complete, previously-declared entity.

namespace cat {
namespace detail {

template <typename Self, typename Callback>
constexpr auto
iterable_pipe_transform(Self&& self, Callback callback) {
   return $fwd(self) | transform(move(callback));
}

template <typename Self, typename Callback>
constexpr auto
iterable_pipe_filter(Self&& self, Callback callback) {
   return $fwd(self) | filter(move(callback));
}

template <typename Self>
constexpr auto
iterable_pipe_take(Self&& self, idx count) {
   return $fwd(self) | take(count);
}

template <typename Self>
constexpr auto
iterable_pipe_reverse(Self&& self) {
   return $fwd(self) | reverse();
}

template <typename Self>
constexpr auto
iterable_pipe_as_rvalue(Self&& self) {
   return $fwd(self) | as_rvalue();
}

template <typename Self, typename Position>
constexpr auto
iterable_pipe_read_at(Self& self, Position const& position) -> decltype(auto) {
   return read_at(self, position);
}

template <typename Self, typename Position>
constexpr auto
iterable_pipe_try_read_at(Self& self, Position const& position) {
   return try_read_at(self, position);
}

template <typename Self, typename Position>
constexpr auto
iterable_pipe_slice(Self& self, Position first, Position last) {
   return slice(self, first, last);
}

template <typename Self>
constexpr auto
iterable_pipe_as_span(Self& self) {
   return as_span(self);
}

template <typename Self>
constexpr auto
iterable_pipe_sum(Self&& self) {
   return $fwd(self) | sum();
}

template <typename Self>
constexpr auto
iterable_pipe_product(Self&& self) {
   return $fwd(self) | product();
}

template <typename Self>
constexpr auto
iterable_pipe_min(Self&& self) {
   return $fwd(self) | min();
}

template <typename Self>
constexpr auto
iterable_pipe_max(Self&& self) {
   return $fwd(self) | max();
}

template <typename Self, typename Other>
constexpr auto
iterable_pipe_dot(Self&& self, Other&& other) {
   return $fwd(self) | dot($fwd(other));
}

template <typename Self>
constexpr auto
iterable_pipe_count(Self&& self) -> idx {
   return $fwd(self) | count();
}

template <typename Self>
constexpr auto
iterable_pipe_popcount(Self&& self) -> idx {
   return $fwd(self) | popcount();
}

template <typename Container, typename Self>
constexpr auto
iterable_pipe_to(Self&& self) -> Container {
   return $fwd(self) | to<Container>();
}

template <typename Self, typename Callback>
constexpr auto
iterable_pipe_for_each(Self&& self, Callback&& callback) -> Callback {
   return $fwd(self) | for_each($fwd(callback));
}

template <typename Self, typename Callback, typename Init>
constexpr auto
iterable_pipe_fold(Self&& self, Callback callback, Init init) -> Init {
   return $fwd(self) | fold(move(callback), move(init));
}

}  // namespace detail

template <typename Tag>
template <typename Self, typename Callback>
constexpr auto
iterable_interface<Tag>::transform(this Self&& self, Callback callback) {
   return detail::iterable_pipe_transform($fwd(self), move(callback));
}

template <typename Tag>
template <typename Self, typename Callback>
constexpr auto
iterable_interface<Tag>::filter(this Self&& self, Callback callback) {
   return detail::iterable_pipe_filter($fwd(self), move(callback));
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::take(this Self&& self, idx count) {
   return detail::iterable_pipe_take($fwd(self), count);
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::reverse(this Self&& self) {
   return detail::iterable_pipe_reverse($fwd(self));
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::as_rvalue(this Self&& self) {
   return detail::iterable_pipe_as_rvalue($fwd(self));
}

template <typename Tag>
template <typename Self, typename Position>
constexpr decltype(auto)
iterable_interface<Tag>::read_at(this Self& self, Position const& position) {
   return detail::iterable_pipe_read_at(self, position);
}

template <typename Tag>
template <typename Self, typename Position>
constexpr auto
iterable_interface<Tag>::try_read_at(
   this Self& self, Position const& position
) {
   return detail::iterable_pipe_try_read_at(self, position);
}

template <typename Tag>
template <typename Self, typename Position>
constexpr auto
iterable_interface<Tag>::slice(this Self& self, Position first, Position last) {
   return detail::iterable_pipe_slice(self, first, last);
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::as_span(this Self& self) {
   return detail::iterable_pipe_as_span(self);
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::sum(this Self&& self) {
   return detail::iterable_pipe_sum($fwd(self));
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::product(this Self&& self) {
   return detail::iterable_pipe_product($fwd(self));
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::min(this Self&& self) {
   return detail::iterable_pipe_min($fwd(self));
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::max(this Self&& self) {
   return detail::iterable_pipe_max($fwd(self));
}

template <typename Tag>
template <typename Self, typename Other>
constexpr auto
iterable_interface<Tag>::dot(this Self&& self, Other&& other) {
   return detail::iterable_pipe_dot($fwd(self), $fwd(other));
}

template <typename Tag>
template <typename Self>
constexpr auto
iterable_interface<Tag>::count(this Self&& self) -> idx {
   return detail::iterable_pipe_count($fwd(self));
}

template <typename Tag>
template <typename Container, typename Self>
constexpr auto
iterable_interface<Tag>::to(this Self&& self) -> Container {
   return detail::iterable_pipe_to<Container>($fwd(self));
}

template <typename Tag>
template <typename Self, typename Callback>
constexpr auto
iterable_interface<Tag>::for_each(this Self&& self, Callback&& callback)
   -> Callback {
   return detail::iterable_pipe_for_each($fwd(self), $fwd(callback));
}

template <typename Tag>
template <typename Self, typename Callback, typename Init>
constexpr auto
iterable_interface<Tag>::fold(this Self&& self, Callback callback, Init init)
   -> Init {
   return detail::iterable_pipe_fold($fwd(self), move(callback), move(init));
}

}  // namespace cat
