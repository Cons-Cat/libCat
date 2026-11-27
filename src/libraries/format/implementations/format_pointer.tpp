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
      // One hex digit per 4 bits, and at least one digit for null.
      idx const digit_count =
         (__builtin_stdc_bit_width(make_raw_arithmetic(integer) | 1u) + 3u)
         / 4u;

      // `0x` and the digits are written straight into the buffer, from the
      // end, so that they come out in order.
      maybe<char*> const p_output =
         context.buffer().prepare_append(idx(digit_count + 2u));
      if (!p_output.has_value()) {
         return format_errors::out_of_memory;
      }

      char* const p_begin = p_output.value();
      p_begin[0] = '0';
      p_begin[1] = 'x';
      for (idx i = digit_count + 1u; i > 1u; i = idx(i - 1u)) {
         p_begin[i.raw] =
            "0123456789abcdef"[make_raw_arithmetic(integer & 0xfu)];
         integer >>= 4u;
      }
      context.buffer().commit(idx(digit_count + 2u));

      return monostate;
   }

   auto
   format(allocator_dyn allocator, T value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
