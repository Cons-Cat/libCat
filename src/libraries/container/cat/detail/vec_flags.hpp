// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/math>

#include "./container_flags.hpp"

namespace cat {

struct vec_flags {
   container_flags const container = {};
   bool const uses_pointer_size_layout = false;
   bool const is_fixed_size = false;
   idx const initial_growth_count = 0u;

   static vec_flags const pointer_size_layout;
   static vec_flags const fixed_size;

   // TODO: We need a flag for storing size and capacity in the allocation next
   // to a VLA that move alongside reallocation.

   constexpr vec_flags() = default;

   constexpr vec_flags(container_flags flags) noexcept
       : container(move(flags)) {
   }

   [[nodiscard]]
   static constexpr auto
   initial_growth(idx count) noexcept -> vec_flags {
      return vec_flags{{}, false, false, count};
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(vec_flags left, vec_flags right) noexcept -> vec_flags {
      container_flags const container = left.container | right.container;
      idx inline_storage_count = container.inline_storage_count;
      idx initial_growth_count = left.initial_growth_count;
      // Favor the right side.
      if (right.initial_growth_count != 0u) {
         inline_storage_count = 0u;
         initial_growth_count = right.initial_growth_count;
      } else if (right.container.inline_storage_count != 0u) {
         initial_growth_count = 0u;
      }

      return vec_flags{
         container_flags{
                         .inline_storage_count = inline_storage_count,
                         .minimum_alignment = container.minimum_alignment,
                         },
         left.uses_pointer_size_layout || right.uses_pointer_size_layout,
         left.is_fixed_size || right.is_fixed_size,
         initial_growth_count,
      };
   }

 private:
   constexpr vec_flags(
      container_flags flags, bool pointer_size_layout, bool fixed_size,
      idx growth_count
   ) noexcept
       : container(move(flags)),
         uses_pointer_size_layout(pointer_size_layout),
         is_fixed_size(fixed_size),
         initial_growth_count(growth_count) {
   }
};

inline constexpr vec_flags vec_flags::pointer_size_layout{{}, true, false, 0u};

inline constexpr vec_flags vec_flags::fixed_size{{}, false, true, 0u};

namespace detail {

// The default `inline_storage_count` should be however many `T` fit in a
// 64-byte `cat::basic_vec`. This size was borrowed from LLVM, which
// supposedly made the evidence-based determination that it's overall optimal.
template <typename T>
consteval auto
default_inline_count() noexcept -> idx {
   if (sizeof(T) > 256u) {
      return 0u;
   }

   idx const size_bytes = sizeof(T) < 4u ? 8u : 4u;
   idx const header = min(
      align_up(idx(sizeof(void*) + size_bytes + size_bytes), alignof(T)),
      idx(64u)
   );
   return max(idx(64u - header) / sizeof(T), 1u);
}

// Set flags for the `vec<T, flags>` alias. When `flags` chooses neither inline
// storage nor growth, `T` gets an inline capacity, or grows like `vec_big<T>`
// if none fits. Otherwise `T` is allowed to be incomplete.
template <typename T, vec_flags flags>
consteval auto
vec_alias_flags() noexcept -> vec_flags {
   if constexpr (
      flags.container.inline_storage_count != 0u
      || flags.initial_growth_count != 0u
   ) {
      return flags;
   } else if constexpr (default_inline_count<T>() == 0u) {
      return vec_flags::initial_growth(4u) | flags;
   } else {
      return container_flags::inline_storage(default_inline_count<T>()) | flags;
   }
}

}  // namespace detail

}  // namespace cat
