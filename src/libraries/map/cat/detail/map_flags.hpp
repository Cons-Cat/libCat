// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/container_flags.hpp>

namespace cat {

struct map_flags {
   container_flags const container = {};

   constexpr map_flags() = default;

   constexpr map_flags(container_flags flags) noexcept
       : container(move(flags)) {
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(map_flags left, map_flags right) noexcept -> map_flags {
      return map_flags{left.container | right.container};
   }
};

}  // namespace cat
