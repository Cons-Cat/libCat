// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/scan_str>
#include <cat/vec>

// `<cat/vec>` is included by `<cat/scan_str>`, so these specializations cannot
// be defined under `basic_vec`. They are declared there and defined here.

namespace cat {

namespace detail {

constexpr auto
trim_list_whitespace(str_view input [[clang::lifetimebound]]) -> str_view {
   // TODO: I feel like this loop might be suboptimal. Look into it. Does this
   // vectorize at least?
   while (input.size() > 0u && is_whitespace(input.front())) {
      input = input.remove_prefix(1u);
   }

   while (input.size() > 0u && is_whitespace(input.back())) {
      input = input.remove_suffix(1u);
   }

   return input;
}

// Find the comma that ends the element starting at `start`. Commas inside an
// element that opens with a `"` or `'` quote are skipped.
constexpr auto
find_list_comma(str_view input, idx start) -> maybe<idx> {
   idx i = start;
   while (i < input.size() && is_whitespace(input[i])) {
      ++i;
   }
   if (i < input.size() && (input[i] == '"' || input[i] == '\'')) {
      char const quote = input[i];
      for (++i; i < input.size(); ++i) {
         if (input[i] == '\\') {
            ++i;
         } else if (input[i] == quote) {
            ++i;
            break;
         }
      }
   }
   return input.find(',', i);
}

template <typename T, is_allocator Allocator, typename Reserve, typename Push>
constexpr auto
vec_scan_comma_list(
   allocator_ref<Allocator> allocator, str_view input, Reserve&& reserve,
   Push&& push
) -> maybe<void> {
   input = trim_list_whitespace(input);

   // Brackets are optional, so that both `1, 2` and `fmt()`'s `[1, 2]` scan.
   if (input.size() != 0u && input.front() == '[') {
      if (input.back() != ']') {
         return nullopt;
      }
      input = trim_list_whitespace(input.substring(1u, idx(input.size() - 2u)));
   }

   if (input.size() == 0u) {
      return monostate;
   }

   // Commas inside quotes are counted too, so this may over-reserve.
   idx count = 1u;
   for (char const character : input) {
      count += idx(character == ',');
   }
   $prop(reserve(count));

   idx start = 0u;
   while (true) {
      maybe<idx> const comma = find_list_comma(input, start);
      idx const end = comma.has_value() ? comma.value() : input.size();

      $prop(push($prop(
         scanner<T>{}.scan(
            allocator,
            trim_list_whitespace(input.substring(start, idx(end - start)))
         )
      )));

      if (!comma.has_value()) {
         return monostate;
      }

      start = end + 1u;
   }
}

}  // namespace detail

template <typename T, vec_flags flags>
struct scanner<basic_vec<T, flags>> {
   // Construct a `vec` out of comma-separated `T` text.
   // TODO: Can we abstract this out for arbitrary collection containers?

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   scan(
      allocator_ref<Allocator> allocator [[clang::lifetimebound]],
      str_view input
   ) const -> maybe<basic_vec<T, flags>> {
      basic_vec<T, flags> result;

      if (
         detail::vec_scan_comma_list<T>(
            allocator, input,
            [&](idx count) {
               return result.reserve(allocator, count);
            },

            [&](T&& value) {
               return result.push_back(allocator, move(value));
            }
         ).is_empty()
      ) {
         result.free(allocator);
         return nullopt;
      }

      return result;
   }
};

template <typename T, is_allocator Allocator, vec_flags flags>
struct scanner<raii::basic_vec<T, Allocator, flags>> {
   [[nodiscard]]
   constexpr auto
   scan(
      allocator_ref<Allocator> allocator [[clang::lifetimebound]],
      str_view input
   ) const -> maybe<raii::basic_vec<T, Allocator, flags>> {
      raii::basic_vec<T, Allocator, flags> result(allocator);

      $prop(
         detail::vec_scan_comma_list<T>(
            allocator, input,
            [&](idx count) {
               return result.reserve(count);
            },
            [&](T&& value) {
               return result.push_back(move(value));
            }
         )
      );

      return result;
   }
};

}  // namespace cat
