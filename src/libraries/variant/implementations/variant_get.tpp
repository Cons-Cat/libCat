// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/variant>

namespace cat {

namespace detail {

template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <idx index>
constexpr auto
variant_base<Alternatives...>::get(
   this auto&& self [[clang::lifetimebound]]
) noexcept(!__has_feature(cxx_exceptions)) -> decltype(auto)
   requires(index < sizeof...(Alternatives))
{
   cat::assert(self.index().value_or_niche() == index);
   if constexpr (is_reference_variant) {
      return *detail::variant_ref_slot<index>($fwd(self).m_storage);
   } else {
      return $fwd(self).m_storage.template get<index>();
   }
}

template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <typename T>
constexpr auto
variant_base<Alternatives...>::get(
   this auto&& self [[clang::lifetimebound]]
) noexcept(!__has_feature(cxx_exceptions)) -> decltype(auto)
   requires(
      types::template is_unique<T>
      || (is_reference_variant && types::template is_unique<add_lvalue_reference<T>>)
   )
{
   if constexpr (types::template has_type<T>) {
      return $fwd(self).as_base().template get<alternative_index<T>>();
   } else {
      return $fwd(self)
         .as_base()
         .template get<alternative_index<add_lvalue_reference<T>>>();
   }
}

template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <typename T>
constexpr auto
variant_base<Alternatives...>::get_ptr(
   this auto&& self [[clang::lifetimebound]]
)
   requires(
      types::template is_unique<T>
      || (is_reference_variant && types::template is_unique<add_lvalue_reference<T>>)
   )
{
   using key =
      conditional<types::template has_type<T>, T, add_lvalue_reference<T>>;
   auto&& base = $fwd(self).as_base();
   using result_type = decltype(__builtin_addressof(base.template get<key>()));
   if (base.template is<key>()) {
      return __builtin_addressof(base.template get<key>());
   }
   return result_type{nullptr};
}

template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <idx index>
constexpr auto
variant_base<Alternatives...>::get_ptr(
   this auto&& self [[clang::lifetimebound]]
)
   requires(index < sizeof...(Alternatives))
{
   auto&& base = $fwd(self).as_base();
   using result_type =
      decltype(__builtin_addressof(base.template get<index>()));
   // `.value_or_niche()` reads the raw discriminant bits. The niche cannot ever
   // matche an alternative index, so a single comparison handles both
   // states.
   if (base.index().value_or_niche() == index) {
      return __builtin_addressof(base.template get<index>());
   }
   return result_type{nullptr};
}

template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <typename T>
constexpr auto
variant_base<Alternatives...>::get_if(this auto&& self [[clang::lifetimebound]])
   requires(types::template is_unique<T> && !is_reference<T>)
{
   auto&& base = $fwd(self).as_base();
   using maybe_type = maybe<decltype(base.template get<T>())>;
   if (base.template is<T>()) {
      return maybe_type{base.template get<T>()};
   }
   return maybe_type{nullopt};
}

template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <typename T>
constexpr auto
variant_base<Alternatives...>::get_if(this auto&& self [[clang::lifetimebound]])
   -> maybe<add_lvalue_reference<T>>
   requires(
      is_reference_variant && !types::template has_type<T>
      && types::template is_unique<add_lvalue_reference<T>>
   )
{
   using key = add_lvalue_reference<T>;
   auto&& base = $fwd(self).as_base();
   if (base.template is<key>()) {
      return base.template get<key>();
   }
   return nullopt;
}

template <typename... Alternatives>
   requires(is_valid_variant_alternatives<Alternatives...>)
template <idx index>
constexpr auto
variant_base<Alternatives...>::get_if(this auto&& self [[clang::lifetimebound]])
   requires(index < sizeof...(Alternatives))
{
   using key = types::template get<index>;
   auto&& base = $fwd(self).as_base();
   // `.value_or_niche()` reads the raw discriminant bits. The niche cannot ever
   // matche an alternative index, so a single comparison handles both
   // states.
   if constexpr (is_reference<key>) {
      using maybe_type = maybe<key>;
      if (base.index().value_or_niche() == index) {
         return maybe_type{base.template get<index>()};
      }
      return maybe_type{nullopt};
   } else {
      using maybe_type = maybe<decltype(base.template get<index>())>;
      if (base.index().value_or_niche() == index) {
         return maybe_type{base.template get<index>()};
      }
      return maybe_type{nullopt};
   }
}

}  // namespace detail

// Free function `cat::get<T>(variant)` mirrors `variant.template get<T>()`,
// and accepts derived types via `is_variant_like` (P2162).
template <typename T, is_variant_like Variant>
   requires(!remove_cvref<Variant>::types::is_all_references)
[[nodiscard]]
constexpr auto
get(Variant&& variant [[clang::lifetimebound]]) -> decltype(auto) {
   return $fwd(variant).template get<T>();
}

template <typename T, is_variant_like Variant>
   requires(
      !is_reference<Variant> && remove_cvref<Variant>::types::is_all_references
   )
[[nodiscard]]
constexpr auto
get(Variant variant [[clang::lifetimebound]]) noexcept(
   !__has_feature(cxx_exceptions)
) -> decltype(auto) {
   return variant.template get<T>();
}

template <idx index, is_variant_like Variant>
   requires(!remove_cvref<Variant>::types::is_all_references)
[[nodiscard]]
constexpr auto
get(Variant&& variant [[clang::lifetimebound]]) -> decltype(auto) {
   return $fwd(variant).template get<index>();
}

template <idx index, is_variant_like Variant>
   requires(
      !is_reference<Variant> && remove_cvref<Variant>::types::is_all_references
   )
[[nodiscard]]
constexpr auto
get(Variant variant [[clang::lifetimebound]]) noexcept(
   !__has_feature(cxx_exceptions)
) -> decltype(auto) {
   return variant.template get<index>();
}

template <typename T, is_variant_like Variant>
   requires(!remove_cvref<Variant>::types::is_all_references)
[[nodiscard]]
constexpr auto
get_if(Variant&& variant [[clang::lifetimebound]]) {
   return $fwd(variant).template get_if<T>();
}

template <typename T, is_variant_like Variant>
   requires(
      !is_reference<Variant> && remove_cvref<Variant>::types::is_all_references
   )
[[nodiscard]]
constexpr auto
get_if(Variant variant [[clang::lifetimebound]]) noexcept {
   return variant.template get_if<T>();
}

template <idx index, is_variant_like Variant>
   requires(!remove_cvref<Variant>::types::is_all_references)
[[nodiscard]]
constexpr auto
get_if(Variant&& variant [[clang::lifetimebound]]) {
   return $fwd(variant).template get_if<index>();
}

template <idx index, is_variant_like Variant>
   requires(
      !is_reference<Variant> && remove_cvref<Variant>::types::is_all_references
   )
[[nodiscard]]
constexpr auto
get_if(Variant variant [[clang::lifetimebound]]) noexcept {
   return variant.template get_if<index>();
}

// P4189 free function `cat::get_ptr<T>(variant)`.
template <typename T, is_variant_like Variant>
   requires(!remove_cvref<Variant>::types::is_all_references)
[[nodiscard]]
constexpr auto
get_ptr(Variant&& variant [[clang::lifetimebound]]) {
   return $fwd(variant).template get_ptr<T>();
}

template <typename T, is_variant_like Variant>
   requires(
      !is_reference<Variant> && remove_cvref<Variant>::types::is_all_references
   )
[[nodiscard]]
constexpr auto
get_ptr(Variant variant [[clang::lifetimebound]]) noexcept {
   return variant.template get_ptr<T>();
}

template <idx index, is_variant_like Variant>
   requires(!remove_cvref<Variant>::types::is_all_references)
[[nodiscard]]
constexpr auto
get_ptr(Variant&& variant [[clang::lifetimebound]]) {
   return $fwd(variant).template get_ptr<index>();
}

template <idx index, is_variant_like Variant>
   requires(
      !is_reference<Variant> && remove_cvref<Variant>::types::is_all_references
   )
[[nodiscard]]
constexpr auto
get_ptr(Variant variant [[clang::lifetimebound]]) noexcept {
   return variant.template get_ptr<index>();
}

template <typename T, is_variant_like Variant>
[[nodiscard]]
constexpr auto
holds_alternative(Variant const& variant) -> bool {
   return variant.template holds_alternative<T>();
}

}  // namespace cat
