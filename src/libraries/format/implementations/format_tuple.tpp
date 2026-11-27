// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <typename... Types, typename CharT>
struct formatter<tuple<Types...>, CharT> : formatter_base<CharT> {
 private:
   tuple<formatter<Types>...> m_underlying;

   template <idx... indices>
   constexpr void
   set_element_debug_format(index_list_type<indices...> /*unused*/, bool set) {
      auto set_one = [&]<idx index> {
         detail::maybe_set_debug_format(
            m_underlying.template get<index>(), set
         );
      };
      (set_one.template operator()<indices>(), ...);
   }

   template <idx... indices, is_allocator Allocator>
   auto
   format_elements(
      allocator_ref<Allocator> allocator, tuple<Types...> const& value,
      format_context context, index_list_type<indices...>
      /*unused*/
   ) const -> scaredy_format<void> {
      scaredy_format<void> result = monostate;

      auto format_one = [&]<idx index> {
         if (result.is_empty()) {
            return;
         }

         if constexpr (index != 0u) {
            result = context.append(", ");
         }

         if (!result.is_empty()) {
            result = m_underlying.template get<index>().format(
               allocator, value.template get<index>(), context
            );
         }
      };

      (format_one.template operator()<indices>(), ...);

      return result;
   }

 public:
   constexpr void
   set_debug_format(bool set = true) {
      set_element_debug_format(index_sequence_over_types<Types...>{}, set);
   }

   constexpr auto
   parse(format_parse_context& context)
      -> scaredy_format<format_parse_context::const_iterator> {
      scaredy_format<format_parse_context::const_iterator> result =
         formatter_base<CharT>::parse(context);
      if (result.is_empty()) {
         return result;
      }

      set_debug_format();
      return result;
   }

   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, tuple<Types...> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      scaredy_format<void> result = context.append('(');
      if (result.is_empty()) {
         return result;
      }

      result = format_elements(
         allocator, value, context, index_sequence_over_types<Types...>{}
      );

      if (result.is_empty()) {
         return result;
      }

      return context.append(')');
   }

   auto
   format(
      allocator_dyn allocator, tuple<Types...> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
