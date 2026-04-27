// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {

// `cat::iterable_interface` is the mixin that turns the pipe-only
// surface (`data | cat::filter(position) | cat::sum()`) into a fluent API
// surface (`data.filter(position).sum()`). It is inherited by every adaptor in
// `<cat/iterable>`.
struct iterable_interface {
   // These are implemented in
   // `<cat/iterable/implementations/iterable_interface.tpp>`.
   template <typename Self, typename Callback>
   constexpr auto
   transform(this Self&& self, Callback callback);

   template <typename Self, typename Callback>
   constexpr auto
   filter(this Self&& self, Callback callback);

   template <typename Self>
   constexpr auto
   take(this Self&& self, idx count);

   template <typename Self>
   constexpr auto
   reverse(this Self&& self);

   template <typename Self>
   constexpr auto
   sum(this Self&& self);

   template <typename Self>
   constexpr auto
   count(this Self&& self) -> idx;

   template <typename Self, typename Callback>
   constexpr auto
   for_each(this Self&& self, Callback callback) -> Callback;

   template <typename Self, typename Callback, typename Init>
   constexpr auto
   fold(this Self&& self, Callback callback, Init init) -> Init;
};

}  // namespace cat

namespace cat::detail {
// `contiguous_position_interface<Derived>` implements the random-access
// position protocol from `.data()` and `.size()`.
template <typename Derived>
struct contiguous_position_interface {
   constexpr auto
   begin_pos(this Derived const& /*self*/) -> idx {
      return 0u;
   }

   constexpr auto
   end_pos(this Derived const& self) -> idx {
      return self.size();
   }

   constexpr void
   inc_pos(this Derived const& /*self*/, idx& position) {
      ++position;
   }

   constexpr void
   dec_pos(this Derived const& /*self*/, idx& position) {
      // `cat::idx` deletes `operator--` to forbid silent underflow.
      // dec_pos's invariant is that `position > begin_pos`, so we can
      // cast back to `idx` after subtraction.
      position = narrow_cast<idx>(position - 1u).assert();
   }

   constexpr void
   inc_pos(this Derived const& /*self*/, idx& position, iword count) {
      position = narrow_cast<idx>(position + count).assert();
   }

   constexpr auto
   distance(this Derived const& /*self*/, idx left, idx right) -> iword {
      return right - left;
   }

   constexpr auto
   read_at_unchecked(this Derived& self, idx position) -> decltype(auto) {
      return self.data()[position];
   }

   constexpr auto
   read_at_unchecked(this Derived const& self, idx position) -> decltype(auto) {
      return self.data()[position];
   }
};
}  // namespace cat::detail

namespace cat {

// Provide a random-access position protocol to a collection along with
// helper methods for a fluent API.
template <typename Derived>
struct contiguous_collection_interface
    : iterable_interface,
      detail::contiguous_position_interface<Derived> {};

}  // namespace cat
