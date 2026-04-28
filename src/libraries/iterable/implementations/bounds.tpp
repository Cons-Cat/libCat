// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/debug>
#include <cat/iterable>

namespace cat {

// Position-based reads and slice construction. These are only available on
// collections, not iterables, because you need a position to ask "is this in
// range?" The position arithmetic deliberately uses only < so the same code
// works on any `is_position` type.

// `read_at(c, p)` reads at position `p`, asserting that `p` is in the half-open
// range `[begin_pos, end_pos)`. The unchecked counterpart
// `c.read_at_unchecked(p)` is the basis operation. This wrapper exists so
// library code can opt into bounds checking everywhere with no per-call
// boilerplate.
template <typename T>
   requires(is_collection<T>)
constexpr auto
read_at(T& collection, position_type<T> const& position)
   -> decltype(read_at_unchecked(collection, position)) {
   cat::assert(!(position < begin_pos(collection)));
   cat::assert(position < end_pos(collection));
   return read_at_unchecked(collection, position);
}

// `try_read_at(collection, position)` returns `nullopt` if `position` is out of
// range, and otherwise the same element `read_at` would return. Use this when
// callers prefer a `maybe` over a failing assert.
template <typename T>
   requires(is_collection<T>)
constexpr auto
try_read_at(T& collection, position_type<T> const& position)
   -> maybe<decltype(read_at_unchecked(collection, position))> {
   if (position < begin_pos(collection) || !(position < end_pos(collection))) {
      return nullopt;
   }
   return read_at_unchecked(collection, position);
}

// Returns a non-owning sub-collection over `[first, last)`. This re-uses the
// parent collection's `.inc_pos()`/`.dec_pos()` / `.read_at_unchecked()` so a
// slice keeps every refinement (multipass / bidirectional / random access) of
// its parent. This is how it differs from contiguous views like `cat::span`.
template <typename T>
   requires(is_collection<T>)
constexpr auto
slice(T& collection [[clang::lifetimebound]], position_type<T> first,
      position_type<T> last) -> detail::slice_view<T> {
   // Bounds are verified at construction so an invalid range fails fast instead
   // of turning into an out-of-bounds read later.
   cat::assert(!(last < first));
   cat::assert(!(first < begin_pos(collection)));
   cat::assert(!(end_pos(collection) < last));

   return detail::slice_view<T>{
      {}, __builtin_addressof(collection), first, last};
}

namespace detail {
template <typename T>
concept has_contiguous_storage = requires(T& collection) {
                                    collection.data();
                                    collection.size();
                                 };
}  // namespace detail

// `data()` + `size()` span over a random-access collection.
template <typename T>
   requires(is_random_access_collection<T> && detail::has_contiguous_storage<T>)
constexpr auto
as_span(T& collection) {
   using element = remove_reference<decltype(*collection.data())>;
   return span<element, dynamic_extent>{collection.data(), collection.size()};
}

}  // namespace cat
