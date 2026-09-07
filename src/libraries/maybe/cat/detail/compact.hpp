// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {

struct in_place_type {};

struct monostate_type {
   constexpr monostate_type() = default;

   constexpr
   operator auto() {
   }
};

template <typename T, typename CharT>
struct formatter;

template <typename CharT>
struct formatter<monostate_type, CharT>;

template <typename T, T constant_state>
struct monotype_storage {
   constexpr monotype_storage() = default;

   constexpr monotype_storage(monostate_type& /*unused*/)
       : m_storage(constant_state) {
   }

   constexpr monotype_storage(monostate_type const& /*unused*/)
       : m_storage(constant_state) {
   }

   constexpr monotype_storage(T input) : m_storage(input) {
   }

   constexpr
   operator auto() const {
      return this->m_storage;
   };

   constexpr auto
   operator=(monostate_type /*unused*/)
      -> monotype_storage<T, constant_state>& {
      return *this;
   }

   friend constexpr auto
   operator<=>(
      monotype_storage<T, constant_state> const& self, auto const& rhs
   ) {
      return self.m_storage <=> rhs;
   }

   friend constexpr auto
   operator==(monotype_storage<T, constant_state> const& self, auto const& rhs)
      -> bool {
      return self.m_storage == rhs;
   }

   [[no_unique_address]]
   T m_storage;
};

template <typename T>
class maybe;

template <typename T, auto predicate, auto>
   requires(is_predicate<__typeof_unqual(predicate), T const&>)
struct compact;

template <typename T, auto predicate, is_invocable auto get_nullopt>
   requires(
      is_predicate<__typeof_unqual(predicate), T const&>
      && !predicate(get_nullopt())
   )
struct compact<T, predicate, get_nullopt> {
   using value_type = T;

   static constexpr auto
   has_value(T const& value) -> bool {
      return predicate(value);
   }

   static constexpr auto
   nullopt_state() {
      return get_nullopt();
   }

   consteval compact() = default;
};

template <typename T, auto predicate, auto nullopt_value>
   requires(
      is_predicate<__typeof_unqual(predicate), T const&>
      && !predicate(T{nullopt_value})
   )
struct compact<T, predicate, nullopt_value>
    : compact<T, predicate, [] constexpr -> T {
       return T{nullopt_value};
    }> {};

template <typename T, auto predicate>
struct compact_scaredy {
   using type = T;
   static constexpr auto predicate_function = predicate;

   consteval compact_scaredy() = default;
};

namespace detail {

template <typename T, T constant_state>
consteval auto
is_monostate_storage_impl(
   monotype_storage<T, constant_state>* _Nullable p_storage [[maybe_unused]]
) -> bool {
   return true;
}

consteval auto
is_monostate_storage_impl(auto* _Nullable p_storage [[maybe_unused]]) -> bool {
   return false;
}

template <typename T>
inline constexpr bool is_monostate_storage =
   is_monostate_storage_impl(static_cast<T*>(nullptr));

template <typename T, T in_sentinel>
constexpr auto
sentinel_predicate(T value) -> bool {
   return value != in_sentinel;
}

template <typename T, T (*_Nonnull get_nullopt)()>
constexpr auto
sentinel_predicate_for_callable(T value) -> bool {
   return value != get_nullopt();
}

}  // namespace detail

template <is_structural T, auto value>
using sentinel = compact<
   T, detail::sentinel_predicate<T, static_cast<T>(value)>,
   static_cast<T>(value)>;

template <typename T, T (*_Nonnull get_nullopt)()>
using sentinel_fn = compact<
   T, &detail::sentinel_predicate_for_callable<T, get_nullopt>, get_nullopt>;

inline constexpr in_place_type in_place;
inline constexpr monostate_type monostate;

template <typename T>
inline constexpr bool is_monostate =
   is_same<T, monostate_type> || detail::is_monostate_storage<T>;

template <typename T>
struct default_compact_trait : identity_trait<T> {};

}  // namespace cat
