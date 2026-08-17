// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat {

struct vec_flags {
   bool const uses_pointer_size_layout = false;
   bool const is_fixed_size = false;
   idx const inline_storage_count = 0u;

   static vec_flags const pointer_size_layout;
   static vec_flags const fixed_size;

   // TODO: We need a flag for storing size and capacity in the allocation next
   // to a VLA that move alongside reallocation.

   [[nodiscard]]
   static constexpr auto
   inline_storage(idx count) -> vec_flags {
      return vec_flags{
         .inline_storage_count = count,
      };
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(vec_flags left, vec_flags right) -> vec_flags {
      return vec_flags{
         .uses_pointer_size_layout =
            left.uses_pointer_size_layout || right.uses_pointer_size_layout,
         .is_fixed_size = left.is_fixed_size || right.is_fixed_size,
         .inline_storage_count =
            cat::max(left.inline_storage_count, right.inline_storage_count),
      };
   }
};

inline constexpr vec_flags vec_flags::pointer_size_layout{
   .uses_pointer_size_layout = true,
};

inline constexpr vec_flags vec_flags::fixed_size{
   .is_fixed_size = true,
};

}  // namespace cat
