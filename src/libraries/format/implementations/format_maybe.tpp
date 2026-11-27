// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename T, typename CharT>
struct formatter<maybe<T>, CharT> : formatter_base<CharT> {
   using value_type = __typeof_unqual(declval<typename maybe<T>::value_type>());
   formatter<value_type> underlying;

   // Specifiers apply to the held value, such as `{:?}` on a held string.
   constexpr auto
   parse(format_parse_context& context)
      -> scaredy_format<format_parse_context::const_iterator> {
      return underlying.parse(context);
   }

   constexpr void
   set_debug_format(bool set = true)
      requires(requires { underlying.set_debug_format(set); })
   {
      underlying.set_debug_format(set);
   }

   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, maybe<T> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      if (!value.has_value()) {
         return context.append("nullopt");
      }
      return underlying.format(allocator, value.value(), context);
   }

   auto
   format(
      allocator_dyn allocator, maybe<T> const& value, format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
