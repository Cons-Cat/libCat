// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat {

struct vec_flags {
   bool uses_pointer_size_layout = false;
   idx inline_storage_count = 0u;

   static vec_flags const pointer_size_layout;

   [[nodiscard]]
   static constexpr auto
   inline_storage(idx count) -> vec_flags {
      return vec_flags{
         .uses_pointer_size_layout = false,
         .inline_storage_count = count,
      };
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(vec_flags left, vec_flags right) -> vec_flags {
      return vec_flags{
         .uses_pointer_size_layout =
            left.uses_pointer_size_layout || right.uses_pointer_size_layout,
         .inline_storage_count =
            cat::max(left.inline_storage_count, right.inline_storage_count),
      };
   }
};

inline constexpr vec_flags vec_flags::pointer_size_layout{
   .uses_pointer_size_layout = true,
   .inline_storage_count = 0u,
};

}  // namespace cat
