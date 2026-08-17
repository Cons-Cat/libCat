// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/container>

namespace cat {

struct str_flags {
   bool const is_null_terminated = false;

   constexpr str_flags() = default;

   static str_flags const null_terminated;

   [[nodiscard]]
   friend constexpr auto
   operator|(str_flags left, str_flags right) -> str_flags {
      return str_flags{
         left.is_null_terminated || right.is_null_terminated,
      };
   }

 private:
   constexpr str_flags(bool is_null_terminated)
       : is_null_terminated(is_null_terminated) {
   }
};

inline constexpr str_flags str_flags::null_terminated{true};

struct str_vec_flags {
   str_flags const str;
   vec_flags const vec = {};

   constexpr str_vec_flags() = default;

   constexpr str_vec_flags(str_flags flags) : str(flags) {
   }

   constexpr str_vec_flags(vec_flags flags) : vec(flags) {
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(str_vec_flags left, str_vec_flags right) -> str_vec_flags {
      return str_vec_flags{left.str | right.str, left.vec | right.vec};
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(str_flags left, str_vec_flags right) -> str_vec_flags {
      return str_vec_flags{left | right.str, right.vec};
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(str_vec_flags left, str_flags right) -> str_vec_flags {
      return str_vec_flags{left.str | right, left.vec};
   }

   friend constexpr auto
   operator|(str_flags string_flags, vec_flags vector_flags) -> str_vec_flags;

 private:
   constexpr str_vec_flags(str_flags string_flags, vec_flags vector_flags)
       : str(string_flags), vec(vector_flags) {
   }
};

[[nodiscard]]
constexpr auto
operator|(str_flags string_flags, vec_flags vector_flags) -> str_vec_flags {
   return str_vec_flags{string_flags, vector_flags};
}

}  // namespace cat
