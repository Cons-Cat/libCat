// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

namespace cat {

template <idx extent, typename CharT>
struct formatter<char[extent], CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, char const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      str_view const string(value, extent - 1u);
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }

   auto
   format(
      allocator_dyn allocator, char const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <idx extent, typename CharT>
struct formatter<char const[extent], CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, char const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      str_view const string(value, extent - 1u);
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }

   auto
   format(
      allocator_dyn allocator, char const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <idx extent, typename CharT>
struct formatter<char8_t[extent], CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, char8_t const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      str_view const string = str_view(u8str_view(value, extent - 1u));
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }

   auto
   format(
      allocator_dyn allocator, char8_t const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <idx extent, typename CharT>
struct formatter<char8_t const[extent], CharT> : debug_formatter<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> /*unused*/, char8_t const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      str_view const string = str_view(u8str_view(value, extent - 1u));
      if (this->debug) {
         return detail::append_escaped(context, string, '"');
      }
      return context.append(string);
   }

   auto
   format(
      allocator_dyn allocator, char8_t const (&value)[extent],
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
