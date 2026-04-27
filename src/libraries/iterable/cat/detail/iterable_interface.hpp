// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {

// Forward declare collection operations. Befriended
// by `contiguous_position_interface` so the defaults keep hooks `private`.
// These are implemented in `<cat/iterable>`.
template <typename Collection>
[[nodiscard]]
constexpr auto
begin_pos(Collection const& collection);
template <typename Collection>
[[nodiscard]]
constexpr auto
end_pos(Collection const& collection);
template <typename Collection, typename Position>
constexpr void
inc_pos(Collection const& collection, Position& position);
template <typename Collection, typename Position>
constexpr void
dec_pos(Collection const& collection, Position& position);
template <typename Collection, typename Position>
[[nodiscard]]
constexpr auto
read_at_unchecked(Collection& collection, Position const& position)
   -> decltype(auto);
template <typename Collection, typename Position>
[[nodiscard]]
constexpr auto
read_at_unchecked(Collection const& collection, Position const& position)
   -> decltype(auto);
template <typename Collection, typename Position>
constexpr void
offset_pos(Collection const& collection, Position& position, iword offset);
template <typename Collection, typename Position>
[[nodiscard]]
constexpr auto
distance(Collection const& collection, Position const& from, Position const& to)
   -> iword;

// `cat::iterable_interface` is the mixin that turns the pipe-only
// surface (`data | cat::filter(position) | cat::sum()`) into a fluent API
// surface (`data.filter(position).sum()`). It is implemented on every adaptor
// in `<cat/iterable>`.
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
// position protocol from `.data()` and `.size()`. The hooks are `private`, and
// should be accessed via the above free functions in `<cat/iterable>`.
template <typename Derived>
struct contiguous_position_interface {
 private:
   template <typename Collection>
   friend constexpr auto ::cat::begin_pos(Collection const&);
   template <typename Collection>
   friend constexpr auto ::cat::end_pos(Collection const&);
   template <typename Collection, typename Position>
   friend constexpr void ::cat::inc_pos(Collection const&, Position&);
   template <typename Collection, typename Position>
   friend constexpr void ::cat::dec_pos(Collection const&, Position&);
   template <typename Collection, typename Position>
   friend constexpr auto ::cat::read_at_unchecked(Collection&, Position const&)
      -> decltype(auto);
   template <typename Collection, typename Position>
   friend constexpr auto ::cat::read_at_unchecked(Collection const&,
                                                  Position const&)
      -> decltype(auto);
   template <typename Collection, typename Position>
   friend constexpr void ::cat::offset_pos(Collection const&, Position&, iword);
   template <typename Collection, typename Position>
   friend constexpr auto ::cat::distance(Collection const&, Position const&,
                                         Position const&) -> iword;

   [[nodiscard]]
   constexpr auto
   begin_pos() const -> idx {
      return 0u;
   }

   [[nodiscard]]
   constexpr auto
   end_pos(this Derived const& self) -> idx {
      return self.size();
   }

   constexpr void
   inc_pos(idx& position) const {
      ++position;
   }

   constexpr void
   dec_pos(idx& position) const {
      // `cat::idx` deletes `operator--` to forbid silent underflow.
      // `dec_pos`'s invariant is that `position > begin_pos`, so we can
      // cast back to `idx` after subtraction.
      position = narrow_cast<idx>(position - 1u).assert();
   }

   constexpr void
   offset_pos(idx& position, iword offset) const {
      position = narrow_cast<idx>(position + offset).assert();
   }

   [[nodiscard]]
   constexpr auto
   distance(idx left, idx right) const -> iword {
      return right - left;
   }

   [[nodiscard]]
   constexpr auto
   read_at_unchecked(this Derived& self, idx position) -> decltype(auto) {
      return self.data()[position];
   }

   [[nodiscard]]
   constexpr auto
   read_at_unchecked(this Derived const& self, idx position) -> decltype(auto) {
      return self.data()[position];
   }
};

}  // namespace cat::detail

namespace cat {

// Provide a random-access position protocol plus a fluent `iterable_interface`
// dot-chaining API.
template <typename Derived>
struct contiguous_collection_interface
    : iterable_interface,
      detail::contiguous_position_interface<Derived> {};

}  // namespace cat
