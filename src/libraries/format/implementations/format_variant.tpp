// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename... Types, typename CharT>
struct formatter<variant<Types...>, CharT> : formatter_base<CharT> {
 private:
   template <idx index = 0u, is_allocator Allocator>
   static auto
   format_active(
      allocator_ref<Allocator> allocator, variant<Types...> const& value,
      format_context context
   ) -> scaredy_format<void> {
      if constexpr (index < sizeof...(Types)) {
         if (value.holds_alternative(index)) {
            return formatter<__typeof_unqual(value.template get<index>())>{}
               .format(allocator, value.template get<index>(), context);
         }
         return format_active<index + 1u>(allocator, value, context);
      } else {
         return format_errors::cannot_format_argument;
      }
   }

 public:
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, variant<Types...> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      if (value.is_empty()) {
         return context.append("<empty>");
      }
      return format_active(allocator, value, context);
   }

   auto
   format(
      allocator_dyn allocator, variant<Types...> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
