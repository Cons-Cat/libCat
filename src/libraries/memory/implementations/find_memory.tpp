// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// These definitions follow `maybe` because `<cat/memory>` can only forward
// declare the return type while `maybe` includes `<cat/memory>`.

#include <cat/memory>

namespace cat {

template <is_equality_comparable T>
[[nodiscard]]
constexpr auto
find_memory(T const* _Nonnull p_haystack, T const& needle, idx size)
   -> maybe<idx> {
   if (size == 0u) {
      return nullopt;
   }

   if consteval {
      for (idx position = 0u; position < size; ++position) {
         if (p_haystack[position] == needle) {
            return position;
         }
      }
      return nullopt;
   } else {
      if constexpr (
         is_trivially_equality_comparable<T> || is_same<remove_cv<T>, byte>
      ) {
         if (p_haystack[0] == needle) {
            return 0u;
         }
         return detail::find_memory_impl(
            reinterpret_cast<byte const*>(p_haystack),
            reinterpret_cast<byte const*>(__builtin_addressof(needle)),
            sizeof(T), size
         );
      } else {
         for (idx position = 0u; position < size; ++position) {
            if (p_haystack[position] == needle) {
               return position;
            }
         }
         return nullopt;
      }
   }
}

template <is_equality_comparable T>
[[nodiscard]]
constexpr auto
find_subspan_memory(
   T const* _Nonnull p_haystack, T const* _Nonnull p_needle, idx size,
   idx needle_size
) -> maybe<idx> {
   if (needle_size == 0u) {
      return 0u;
   }
   if (needle_size > size) {
      return nullopt;
   }
   if (needle_size == 1u) {
      return find_memory(p_haystack, p_needle[0], size);
   }

   if consteval {
      for (idx position = 0u; position <= size - needle_size; ++position) {
         bool matches = true;
         for (idx i = 0u; i < needle_size; ++i) {
            if (p_haystack[position + i] != p_needle[i]) {
               matches = false;
               break;
            }
         }
         if (matches) {
            return position;
         }
      }
      return nullopt;
   } else {
      if constexpr (
         is_trivially_equality_comparable<T> || is_same<remove_cv<T>, byte>
      ) {
         if (needle_size <= 16u / sizeof(T)) {
            bool matches = true;
            for (idx i = 0u; i < needle_size; ++i) {
               if (p_haystack[i] != p_needle[i]) {
                  matches = false;
                  break;
               }
            }
            if (matches) {
               return 0u;
            }
            if (size == needle_size) {
               return nullopt;
            }
         }
         return detail::find_subspan_memory_impl(
            reinterpret_cast<byte const*>(p_haystack),
            reinterpret_cast<byte const*>(p_needle), sizeof(T), size,
            needle_size
         );
      } else {
         for (idx position = 0u; position <= size - needle_size; ++position) {
            bool matches = true;
            for (idx i = 0u; i < needle_size; ++i) {
               if (p_haystack[position + i] != p_needle[i]) {
                  matches = false;
                  break;
               }
            }
            if (matches) {
               return position;
            }
         }
         return nullopt;
      }
   }
}

}  // namespace cat
