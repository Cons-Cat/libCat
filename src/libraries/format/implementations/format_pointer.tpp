// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename T, typename CharT>
   requires(
      is_pointer<T> && !is_char_utf8_interconvertible<remove_pointer<T>>
      && is_same<CharT, char>
   )
struct formatter<T, CharT> : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, T value, format_context context
   ) const -> scaredy_format<void> {
      uintptr<remove_pointer<T>> integer = value;
      char digits[sizeof(integer) * 2u];
      idx length = 0u;

      do {
         auto const digit = integer & 0xfu;
         digits[length] =
            static_cast<char>(digit < 10u ? digit + '0' : digit - 10u + 'a');
         ++length;
         integer >>= 4u;
      } while (integer != 0u);

      scaredy_format<void> result = context.append("0x");
      if (result.is_empty()) {
         return result;
      }

      iword position = make_signed(length);
      while (position > 0) {
         --position;
         result = context.append(digits[position.raw]);
         if (result.is_empty()) {
            return result;
         }
      }

      return monostate;
   }

   auto
   format(allocator_dyn allocator, T value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
