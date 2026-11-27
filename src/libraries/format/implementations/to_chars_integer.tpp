// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/itoa_jeaiii.hpp>

#include <cat/arithmetic>
#include <cat/format>
#include <cat/maybe>
#include <cat/meta>
#include <cat/string>

namespace cat {
template <is_integral T>
   requires(!is_bool<T>)
constexpr auto
to_chars(T value) -> str_inplace<detail::to_chars_integer_capacity<T>> {
   auto const raw = make_raw_arithmetic(value);
   using raw_type = __typeof_unqual(raw);

   str_inplace<detail::to_chars_integer_capacity<T>> string;
   // TODO: This needs a `resize_uninit()` that does not fill the digits.
   string.resize(string.capacity()).verify();
   char* const p_begin = string.data();
   char* p_end;

   if constexpr (sizeof(T) <= 4u && is_signed<raw_type>) {
      p_end = detail::i32toa_jeaiii(raw, p_begin);
   } else if constexpr (sizeof(T) <= 4u) {
      p_end = detail::u32toa_jeaiii(raw, p_begin);
   } else if constexpr (sizeof(T) == 8u && is_signed<raw_type>) {
      p_end = detail::i64toa_jeaiii(raw, p_begin);
   } else {
      static_assert(sizeof(T) == 8u);
      p_end = detail::u64toa_jeaiii(raw, p_begin);
   }

   // Shrink to fit the data that was written in.
   string.resize(idx(p_end - p_begin)).verify();
   return string;
}

}  // namespace cat
