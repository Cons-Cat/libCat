// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// P2141R2-style "named tuples". Clang `__builtin_structured_binding_size`,
// `cat::get`/`cat::tuple_size` (analogous to the proposed `std::aggr::get` in
// that paper) for such aggregates, `has_aggregate_get`, and `cat::apply` for
// plain aggregates.

#include <cat/tuple>

namespace cat {
namespace detail {

// "Magic get" for aggregates.
template <idx index, typename T>
   requires(
      __builtin_structured_binding_size(remove_cvref<T>) > 0
      && (index < __builtin_structured_binding_size(remove_cvref<T>))
   )
[[nodiscard, gnu::always_inline]]
constexpr auto
get_aggregate_lvalue(T& aggregate) -> decltype(auto) {
   // Compared to `Boost.pfr` or the `magic_get` library, this is extremely
   // simple.
   // We just destructure into a pack, and then index that pack.
   auto& [... members] = aggregate;
   return (members...[index]);
}

// True when a member `get` is callable like `cat::tuple::get` (used with
// structured bindings on tuple-like class types) so the type is not a
// P2141-style magic aggregate.
template <typename T>
consteval auto
has_member_get_at_index_zero() -> bool {
   using u = remove_cvref<T>;
   if constexpr (requires(u& aggregate) { aggregate.template get<0u>(); }) {
      return true;
   }
   if constexpr (requires(u const& aggregate) {
                    aggregate.template get<0u>();
                 }) {
      return true;
   }
   return false;
}

template <typename T>
inline constexpr bool has_member_get_at_index_zero_v =
   has_member_get_at_index_zero<T>();

template <typename, typename = void>
inline constexpr bool has_std_tuple_size_for_aggregate_get = false;
template <typename T>
inline constexpr bool has_std_tuple_size_for_aggregate_get<
   T, void_type<decltype(std::tuple_size<T>::value)>> = true;

// Tuple-like for magic aggregate access. P2141 path is only for real
// aggregates, not for types that already have the tuple-size protocol or a
// member `.get<>` like `cat::tuple`.
template <typename T>
consteval auto
has_aggregate_get_impl() -> bool {
   using type = remove_cvref<T>;
   // C++ aggregates include array types, not only `class`/`struct`, so
   // `__is_aggregate` does not imply `__is_class`.
   return !has_std_tuple_size_for_aggregate_get<remove_cvref<T>>
          && !has_member_get_at_index_zero_v<type> && __is_class(type)
          && __is_aggregate(type)
          && !is_union<type> && !is_volatile<remove_reference<T>>;
}

// Only used in unevaluated contexts. `decltype` of a structured binding is
// the member's declared type, so this preserves reference members.
template <idx index, typename T>
constexpr auto
aggregate_member_type_list(T& aggregate) {
   auto& [... members] = aggregate;
   return type_list<decltype(members...[index])>{};
}

template <idx index, typename T>
using aggregate_member_type = decltype(aggregate_member_type_list<index>(
   declval<remove_reference<T>&>()
))::template get<0u>;

template <idx index, typename T>
inline constexpr bool is_aggregate_member_reference =
   is_reference<aggregate_member_type<index, T>>;
}  // namespace detail

// Predicate for the libCat P2141 path. Plain aggregates with
// `__builtin_structured_binding_size` in range, and not in the tuple protocol
// (specialized tuple-size or a tuple-like `.get<>` on the type).
template <typename T>
inline constexpr bool has_aggregate_get = detail::has_aggregate_get_impl<T>();

template <typename T>
   requires(has_aggregate_get<remove_cvref<T>>)
struct tuple_size
    : constant<__builtin_structured_binding_size(remove_cvref<T>)> {};

// Analogous to the proposed `std::aggr::get` in the P2141 paper.
// Like `cat::tuple::get`, the result is only `lifetimebound` to the aggregate
// when the member is not a reference.
template <idx index, typename S>
   requires(
      has_aggregate_get<remove_cvref<S>>
      && index < __builtin_structured_binding_size(remove_cvref<S>)
      && !detail::is_aggregate_member_reference<index, S>
   )
[[nodiscard]]
constexpr auto
get(S& aggregate [[clang::lifetimebound]]) noexcept -> decltype(auto) {
   return detail::get_aggregate_lvalue<index>(aggregate);
}

template <idx index, typename S>
   requires(
      has_aggregate_get<remove_cvref<S>>
      && index < __builtin_structured_binding_size(remove_cvref<S>)
      && !detail::is_aggregate_member_reference<index, S>
   )
[[nodiscard]]
constexpr auto
get(S const& aggregate [[clang::lifetimebound]]) noexcept -> decltype(auto) {
   return detail::get_aggregate_lvalue<index>(aggregate);
}

// `S&&` with `is_rvalue_reference`:
// True pr-value or x-value aggregate so we can bind the pack and move the
// selected element out.
template <idx index, typename S>
   requires(
      is_rvalue_reference<S&&> && has_aggregate_get<remove_cvref<S>>
      && index < __builtin_structured_binding_size(remove_cvref<S>)
      && !detail::is_aggregate_member_reference<index, S>
   )
[[nodiscard]]
constexpr auto
get(S&& aggregate [[clang::lifetimebound]]) noexcept -> decltype(move(
   detail::get_aggregate_lvalue<index>(
      static_cast<remove_reference<S>&>(aggregate)
   )
)) {
   return move(
      detail::get_aggregate_lvalue<index>(
         static_cast<remove_reference<S>&>(aggregate)
      )
   );
}

// A reference member refers to storage the aggregate does not own, so it is
// returned as declared for every value category of the aggregate.
template <idx index, typename S>
   requires(
      has_aggregate_get<remove_cvref<S>>
      && index < __builtin_structured_binding_size(remove_cvref<S>)
      && detail::is_aggregate_member_reference<index, S>
   )
[[nodiscard]]
constexpr auto
get(S&& aggregate) noexcept -> detail::aggregate_member_type<index, S> {
   return static_cast<detail::aggregate_member_type<index, S>>(
      detail::get_aggregate_lvalue<index>(
         static_cast<remove_reference<S>&>(aggregate)
      )
   );
}

}  // namespace cat

namespace cat::detail {

// `make_index_sequence` over `cat::idx` to `cat::get` for
// `aggregate_apply_impl`.
template <idx index, typename Aggregate>
   requires(has_aggregate_get<remove_cvref<Aggregate>>)
[[nodiscard, gnu::always_inline]]
constexpr auto
aggregate_get(Aggregate&& aggregate) -> decltype(auto) {
   return get<index>($fwd(aggregate));
}

template <typename Callback, typename Aggregate, idx... indices>
[[nodiscard, gnu::always_inline]]
constexpr auto
aggregate_apply_impl(
   Callback&& callback, Aggregate&& aggregate, index_list_type<indices...>
) -> decltype($fwd(callback)(aggregate_get<indices>($fwd(aggregate))...)) {
   return $fwd(callback)(aggregate_get<indices>($fwd(aggregate))...);
}

}  // namespace cat::detail

namespace cat {

// `apply` for `has_aggregate_get` types, separate overload from
// `is_tuple<>`-based `apply` in the main `tuple` header.
template <typename Callback, typename Aggregate>
   requires(has_aggregate_get<remove_cvref<Aggregate>>)
[[nodiscard]]
constexpr auto
apply(Callback&& callback, Aggregate&& aggregate) {
   return detail::aggregate_apply_impl(
      $fwd(callback), $fwd(aggregate),
      make_index_sequence<static_cast<
         idx>(__builtin_structured_binding_size(remove_cvref<Aggregate>))>{}
   );
}

}  // namespace cat
