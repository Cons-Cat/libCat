// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/math>

namespace cat {

// Flags shared by every container family. `cat::vec_flags` and
// `cat::map_flags` hold these, and implicitly convert from them.
struct container_flags {
   idx const inline_storage_count = 0u;
   // Containers allocate with at least this alignment. `align_*` allocations
   // never go below their element type's.
   ualign const minimum_alignment = 1u;

   [[nodiscard]]
   static constexpr auto
   inline_storage(idx count) noexcept -> container_flags {
      return container_flags{
         .inline_storage_count = count,
      };
   }

   [[nodiscard]]
   static constexpr auto
   alignment(ualign value) noexcept -> container_flags {
      return container_flags{
         .minimum_alignment = value,
      };
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(container_flags left, container_flags right) noexcept
      -> container_flags {
      // Favor the right side.
      return container_flags{
         .inline_storage_count = right.inline_storage_count != 0u
                                    ? right.inline_storage_count
                                    : left.inline_storage_count,
         .minimum_alignment = left.minimum_alignment > right.minimum_alignment
                                 ? left.minimum_alignment
                                 : right.minimum_alignment,
      };
   }
};

}  // namespace cat
