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
iterable_pipe_sum(Self&& self) {
   return sum($fwd(self));
}

template <typename Self>
constexpr auto
iterable_pipe_count(Self&& self) -> idx {
   return count($fwd(self));
}

template <typename Self, typename Callback>
constexpr auto
iterable_pipe_for_each(Self&& self, Callback callback) -> Callback {
   return for_each($fwd(self), move(callback));
}

template <typename Self, typename Callback, typename Init>
constexpr auto
iterable_pipe_fold(Self&& self, Callback callback, Init init) -> Init {
   return fold($fwd(self), move(callback), move(init));
}

}  // namespace detail

template <typename Self, typename Callback>
constexpr auto
iterable_interface::transform(this Self&& self, Callback callback) {
   return detail::iterable_pipe_transform($fwd(self), move(callback));
}

template <typename Self, typename Callback>
constexpr auto
iterable_interface::filter(this Self&& self, Callback callback) {
   return detail::iterable_pipe_filter($fwd(self), move(callback));
}

template <typename Self>
constexpr auto
iterable_interface::take(this Self&& self, idx count) {
   return detail::iterable_pipe_take($fwd(self), count);
}

template <typename Self>
constexpr auto
iterable_interface::reverse(this Self&& self) {
   return detail::iterable_pipe_reverse($fwd(self));
}

template <typename Self>
constexpr auto
iterable_interface::sum(this Self&& self) {
   return detail::iterable_pipe_sum($fwd(self));
}

template <typename Self>
constexpr auto
iterable_interface::count(this Self&& self) -> idx {
   return detail::iterable_pipe_count($fwd(self));
}

template <typename Self, typename Callback>
constexpr auto
iterable_interface::for_each(this Self&& self, Callback callback) -> Callback {
   return detail::iterable_pipe_for_each($fwd(self), move(callback));
}

template <typename Self, typename Callback, typename Init>
constexpr auto
iterable_interface::fold(this Self&& self, Callback callback, Init init)
   -> Init {
   return detail::iterable_pipe_fold($fwd(self), move(callback), move(init));
}

}  // namespace cat
