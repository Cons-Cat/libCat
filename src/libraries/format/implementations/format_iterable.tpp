// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename T, typename CharT>
   requires(is_iterable<T const> && !is_safe_string<T> && is_same<CharT, char>)
struct formatter<T, CharT> : formatter_base<CharT> {
   using element_type = iterable_value_type<T const>;
   formatter<element_type> underlying;

   constexpr void
   set_debug_format(bool set = true)
      requires(requires { underlying.set_debug_format(set); })
   {
      underlying.set_debug_format(set);
   }

   constexpr auto
   parse(format_parse_context& context)
      -> scaredy_format<format_parse_context::const_iterator> {
      scaredy_format<format_parse_context::const_iterator> result =
         formatter_base<CharT>::parse(context);
      if (result.is_empty()) {
         return result;
      }

      detail::maybe_set_debug_format(underlying);
      return result;
   }

   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, T const& value, format_context context
   ) const -> scaredy_format<void> {
      scaredy_format<void> result = context.append('[');

      if (result.is_empty()) {
         return result;
      }

      bool first = true;

      iterate(value).run_while([&](auto const& element) {
         if (!first) {
            result = context.append(", ");
            if (result.is_empty()) {
               return false;
            }
         }

         first = false;
         result = underlying.format(allocator, element, context);
         return !result.is_empty();
      });

      if (result.is_empty()) {
         return result;
      }

      return context.append(']');
   }

   auto
   format(allocator_dyn allocator, T const& value, format_context context) const
      -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
