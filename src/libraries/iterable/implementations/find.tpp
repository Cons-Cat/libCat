// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/str_span.hpp>
#include <cat/iterable>
#include <cat/memory>

namespace cat {
namespace detail {
template <typename T>
struct find_impl {
   T needle;

   template <is_iterable Iterable>
   friend constexpr auto
   operator|(Iterable&& incoming, find_impl self) -> maybe<idx> {
      using element_type = iterable_value_type<Iterable>;

      if constexpr (
         is_contiguous_iterable<Iterable>
         && is_same<remove_cv<T>, element_type>
         && (is_trivially_equality_comparable<element_type>
             || is_same<element_type, byte>)
      ) {
         return find_memory(
            incoming.data(), self.needle, incoming.size()
         );
      } else if constexpr (
         is_contiguous_iterable<Iterable>
         && sizeof(element_type) == 1u
         && is_implicit_lifetime<T>
         && (is_trivially_equality_comparable<element_type>
             || is_same<element_type, byte>)
      ) {
         return find_memory(
            reinterpret_cast<byte const*>(incoming.data()),
            static_cast<byte>(self.needle), incoming.size()
         );
      }

      // Otherwise, fall back to a conventional iterator loop.
      maybe<idx> maybe_position{nullopt};
      idx position = 0u;
      iterate(incoming).run_while(
         [&self, &maybe_position, &position](auto&& element) -> bool {
            if (element == self.needle) {
               maybe_position = position;
               return false;
            }
            ++position;
            return true;
         }
      );
      return maybe_position;
   }
};

template <typename T, bool includes_null>
struct find_subspan_impl {
   span<T const> needle;

   template <is_contiguous_iterable Iterable>
      requires(is_same<iterable_value_type<Iterable>, remove_cv<T>>)
   friend constexpr auto
   operator|(Iterable&& incoming, find_subspan_impl self) -> maybe<idx> {
      idx needle_size = self.needle.size();
      if constexpr (includes_null) {
         needle_size = idx(needle_size - 1u);
      }
      return find_subspan_memory(
         static_cast<remove_cv<T> const*>(incoming.data()),
         static_cast<remove_cv<T> const*>(self.needle.data()),
         incoming.size(), needle_size
      );
   }
};
}  // namespace detail

// Find the position of the first element equal to `needle`, or `nullopt`.
// `needle` is not taken by `const&`, as that can dangle if the view outlives
// the needle. If `T` is non-copyable, pass `cat::ref(needle)` instead.
// This is a terminal algorithm.
template <typename T>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
find(T needle) -> detail::find_impl<T> {
   return detail::find_impl<T>{move(needle)};
}

template <typename T, idx fixed_extent>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
find(
   span<T, fixed_extent> const& needle [[clang::lifetimebound]]
)
   -> detail::find_subspan_impl<remove_cv<T>, false> {
   return {
      span<remove_cv<T> const>{needle.data(), needle.size()},
   };
}

template <typename CharT, str_flags flags>
[[gnu::always_inline, gnu::nodebug]]
constexpr auto
find(
   basic_str_span<CharT, flags> const& needle [[clang::lifetimebound]]
)
   -> detail::find_subspan_impl<
      remove_cv<CharT>, flags.is_null_terminated
   > {
   return {
      span<remove_cv<CharT> const>{needle.data(), needle.size()},
   };
}

template <is_string_char CharT, idx extent>
[[gnu::always_inline, gnu::nodebug]]
consteval auto
find(CharT const (&needle)[extent]) {
   using zview_type =
      basic_str_span<CharT const, str_flags::null_terminated>;
   using result_type = detail::find_subspan_impl<CharT, true>;
   zview_type const zneedle{needle};
   return result_type{
      span<CharT const>{zneedle.data(), zneedle.size()},
   };
}

}  // namespace cat
