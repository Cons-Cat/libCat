// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// `cat::str_flags` configures whether a string type is null-terminated.
// This header is dependency-free so low-level headers such as `<cat/debug>`
// and `<cat/maybe>` can use `str_flags` as a `basic_str_span` template
// parameter without pulling in `str_vec_flags`.

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

}  // namespace cat
