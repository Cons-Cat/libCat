// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat {

struct map_flags {
   idx const inline_storage_count = 0u;

   [[nodiscard]]
   static constexpr auto
   inline_storage(idx count) -> map_flags {
      return map_flags{
         .inline_storage_count = count,
      };
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(map_flags left, map_flags right) -> map_flags {
      // Favor the right side.
      return map_flags{
         .inline_storage_count = right.inline_storage_count != 0u
                                    ? right.inline_storage_count
                                    : left.inline_storage_count,
      };
   }
};

}  // namespace cat
