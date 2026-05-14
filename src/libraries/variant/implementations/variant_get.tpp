// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/variant>

namespace cat {

// Out-of-line definitions for the `get`/`get_if`/`get_ptr` member family
// declared inside `variant<Alternatives...>`. Kept here so the class body
// stays focused on the public surface while these bodies (which lean on
// the shared `as_base` helper, the recursive `variant_node` storage, and
// the per-alternative `maybe` wrappers) remain readable.

template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <idx index>
constexpr auto
variant<Alternatives...>::get(this auto&& self) -> decltype(auto)
   requires(index < sizeof...(Alternatives))
{
   if constexpr (is_reference_variant) {
      return *detail::variant_ref_slot<index>($fwd(self).m_storage);
   } else {
      return $fwd(self).m_storage.template get<index>();
   }
}

template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <typename T>
constexpr auto
variant<Alternatives...>::get(this auto&& self) -> decltype(auto)
   requires(types::template is_unique<T>
            || (is_reference_variant
                && types::template is_unique<add_lvalue_reference<T>>))
{
   if constexpr (types::template has_type<T>) {
      return as_base($fwd(self)).template get<alternative_index<T>>();
   } else {
      return as_base($fwd(self))
         .template get<alternative_index<add_lvalue_reference<T>>>();
   }
}

template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <typename T>
constexpr auto
variant<Alternatives...>::get_ptr(this auto&& self)
   requires(types::template is_unique<T>
            || (is_reference_variant
                && types::template is_unique<add_lvalue_reference<T>>))
{
   using key =
      conditional<types::template has_type<T>, T, add_lvalue_reference<T>>;
   auto&& base = as_base($fwd(self));
   using result_type =
      decltype(__builtin_addressof(base.template get<key>()));
   if (base.template is<key>()) {
      return __builtin_addressof(base.template get<key>());
   }
   return result_type{nullptr};
}

template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <idx index>
constexpr auto
variant<Alternatives...>::get_ptr(this auto&& self)
   requires(index < sizeof...(Alternatives))
{
   auto&& base = as_base($fwd(self));
   using result_type =
      decltype(__builtin_addressof(base.template get<index>()));
   // `.value_or_niche()` reads the raw discriminant bits including the
   // niche when empty. The niche never matches a valid alternative
   // index, so a single comparison handles both states.
   if (base.discriminant.value_or_niche() == index) {
      return __builtin_addressof(base.template get<index>());
   }
   return result_type{nullptr};
}

template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <typename T>
constexpr auto
variant<Alternatives...>::get_if(this auto&& self)
   requires(types::template is_unique<T> && !is_reference<T>)
{
   auto&& base = as_base($fwd(self));
   using maybe_type = maybe<decltype(base.template get<T>())>;
   if (base.template is<T>()) {
      return maybe_type{base.template get<T>()};
   }
   return maybe_type{nullopt};
}

template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <typename T>
constexpr auto
variant<Alternatives...>::get_if(this auto&& self)
   -> maybe<add_lvalue_reference<T>>
   requires(is_reference_variant && !types::template has_type<T>
            && types::template is_unique<add_lvalue_reference<T>>)
{
   using key = add_lvalue_reference<T>;
   auto&& base = as_base($fwd(self));
   if (base.template is<key>()) {
      return base.template get<key>();
   }
   return nullopt;
}

template <typename... Alternatives>
   requires(type_list<Alternatives...>::is_all_references
            || !type_list<Alternatives...>::is_any_reference)
template <idx index>
constexpr auto
variant<Alternatives...>::get_if(this auto&& self)
   requires(index < sizeof...(Alternatives))
{
   using key = types::template get<index>;
   auto&& base = as_base($fwd(self));
   // `.value_or_niche()` reads the raw discriminant bits including the
   // niche when empty. The niche never matches a valid alternative
   // index, so the comparison handles both states.
   if constexpr (is_reference<key>) {
      using maybe_type = maybe<key>;
      if (base.discriminant.value_or_niche() == index) {
         return maybe_type{base.template get<index>()};
      }
      return maybe_type{nullopt};
   } else {
      using maybe_type = maybe<decltype(base.template get<index>())>;
      if (base.discriminant.value_or_niche() == index) {
         return maybe_type{base.template get<index>()};
      }
      return maybe_type{nullopt};
   }
}

// Free function `cat::get<T>(variant)` mirrors `variant.template get<T>()`,
// and accepts derived types via `is_variant_like` (P2162).
template <typename T, is_variant_like Variant>
[[nodiscard]]
constexpr auto
get(Variant&& v) -> auto&& {
   return $fwd(v).template get<T>();
}

template <idx index, is_variant_like Variant>
[[nodiscard]]
constexpr auto
get(Variant&& v) -> auto&& {
   return $fwd(v).template get<index>();
}

template <typename T, is_variant_like Variant>
[[nodiscard]]
constexpr auto
get_if(Variant&& v) {
   return $fwd(v).template get_if<T>();
}

template <idx index, is_variant_like Variant>
[[nodiscard]]
constexpr auto
get_if(Variant&& v) {
   return $fwd(v).template get_if<index>();
}

// P4189 free function `cat::get_ptr<T>(variant)`.
template <typename T, is_variant_like Variant>
[[nodiscard]]
constexpr auto
get_ptr(Variant&& v) {
   return $fwd(v).template get_ptr<T>();
}

template <idx index, is_variant_like Variant>
[[nodiscard]]
constexpr auto
get_ptr(Variant&& v) {
   return $fwd(v).template get_ptr<index>();
}

template <typename T, is_variant_like Variant>
[[nodiscard]]
constexpr auto
holds_alternative(Variant const& v) -> bool {
   return v.template holds_alternative<T>();
}

}  // namespace cat
