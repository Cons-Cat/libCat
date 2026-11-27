// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>
#include <cat/scaredy>

// `<cat/scaredy>` is included by `<cat/format>`, so this specialization cannot
// be defined under `scaredy`. It is declared there and defined here after
// `formatter_base` exists.

namespace cat {

template <typename T, typename... Errors, typename CharT>
struct formatter<scaredy<T, Errors...>, CharT> : formatter_base<CharT> {
 private:
   template <typename Error, is_allocator Allocator>
   static auto
   try_format_error(
      allocator_ref<Allocator> allocator, scaredy<T, Errors...> const& value,
      format_context context, bool& done
   ) -> scaredy_format<void> {
      if (done || !value.template is<Error>()) {
         return monostate;
      }
      done = true;
      return formatter<__typeof_unqual(value.template error<Error>())>{}.format(
         allocator, value.template error<Error>(), context
      );
   }

 public:
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator, scaredy<T, Errors...> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      if (value.has_value()) {
         scaredy_format<void> result = context.append("ok(");
         if (result.is_empty()) {
            return result;
         }
         if constexpr (requires { value.value(); }) {
            result = formatter<__typeof_unqual(value.value())>{}.format(
               allocator, value.value(), context
            );
            if (result.is_empty()) {
               return result;
            }
         }
         return context.append(')');
      }

      scaredy_format<void> result = context.append("err(");
      if (result.is_empty()) {
         return result;
      }
      bool done = false;
      scaredy_format<void> error_result = monostate;
      auto attempt = [&]<typename Error> {
         if (error_result.is_empty() || done) {
            return;
         }
         error_result =
            try_format_error<Error>(allocator, value, context, done);
      };
      (attempt.template operator()<Errors>(), ...);
      if (error_result.is_empty()) {
         return error_result;
      }
      if (!done) {
         return format_errors::cannot_format_argument;
      }
      return context.append(')');
   }

   auto
   format(
      allocator_dyn allocator, scaredy<T, Errors...> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
