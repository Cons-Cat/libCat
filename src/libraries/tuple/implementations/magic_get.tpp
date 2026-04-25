// -*- mode: c++ -*-
// vim: set ft=cpp:
// P2141R2-style "named tuples": Clang `__builtin_structured_binding_size`,
// `cat::get` / `cat::tuple_size` (analogous to the proposed `std::aggr::get` in
// that paper) for such aggregates, `has_aggregate_get`, and `cat::apply` for
// plain aggregates

namespace cat {
namespace detail {

// "Magic get" for aggregates.
template <idx in_index, typename T>
   requires(__builtin_structured_binding_size(remove_cvref<T>) > 0
            && (in_index < __builtin_structured_binding_size(remove_cvref<T>)))
[[nodiscard, gnu::always_inline]]
constexpr auto
get_aggregate_lvalue(T& t) -> decltype(auto) {
   // Compared to Boost.pfr or the magic_get library, this is extremely simple.
   // We just destructure into a pack, and then index that pack.
   auto& [... members] = t;
   return (members...[in_index]);
}

// True when a member `get` is callable like `cat::tuple::get` (used with
// structured bindings on tuple-like class types) so the type is not a
// P2141-style magic aggregate.
template <typename T>
consteval auto
has_member_get_at_index_zero() -> bool {
   using u = remove_cvref<T>;
   if constexpr (requires(u& t) { t.template get<0u>(); }) {
      return true;
   }
   if constexpr (requires(u const& t) { t.template get<0u>(); }) {
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

// Tuple-like for magic aggregate access: P2141 path is only for real
// aggregates, not for types that already have the tuple-size protocol or
// a member `.get<>` like `cat::tuple`
template <typename T>
consteval auto
has_aggregate_get_impl() -> bool {
   using type = remove_cvref<T>;
   // C++ aggregates include array types, not only `class` / `struct`, so
   // `__is_aggregate` does not imply `__is_class`
   return !has_std_tuple_size_for_aggregate_get<remove_cvref<T>>
          && !has_member_get_at_index_zero_v<type> && __is_class(type)
          && __is_aggregate(type)
          && !is_union<type> && !is_volatile<remove_reference<T>>;
}
}  // namespace detail

// Predicate for the libCat P2141 path: plain aggregates with
// `__builtin_structured_binding_size` in range, and not in the tuple protocol
// (specialized tuple-size or a tuple-like `.get<>` on the type)
template <typename T>
inline constexpr bool has_aggregate_get = detail::has_aggregate_get_impl<T>();

template <typename T>
   requires(has_aggregate_get<remove_cvref<T>>)
struct tuple_size
    : constant<__builtin_structured_binding_size(remove_cvref<T>)> {};

// Analogous to the proposed `std::aggr::get` in the P2141 paper
template <idx in_index, typename S>
   requires(has_aggregate_get<remove_cvref<S>>
            && in_index
                  < __builtin_structured_binding_size(::cat::remove_cvref<S>))
[[nodiscard]]
constexpr auto
get(S& t) noexcept -> decltype(auto) {
   return ::cat::detail::get_aggregate_lvalue<in_index>(t);
}

template <idx in_index, typename S>
   requires(has_aggregate_get<remove_cvref<S>>
            && in_index
                  < __builtin_structured_binding_size(::cat::remove_cvref<S>))
[[nodiscard]]
constexpr auto
get(S const& t) noexcept -> decltype(auto) {
   return ::cat::detail::get_aggregate_lvalue<in_index>(t);
}

// `S&&` with `is_rvalue_reference`: true prvalue or xvalue aggregate so we
// can bind the pack and move the selected element out
template <idx in_index, typename S>
   requires(is_rvalue_reference<S &&> && has_aggregate_get<remove_cvref<S>>
            && in_index
                  < __builtin_structured_binding_size(::cat::remove_cvref<S>))
[[nodiscard]]
constexpr auto
get(S&& t) noexcept
   -> decltype(cat::move(::cat::detail::get_aggregate_lvalue<in_index>(
      static_cast<remove_reference<S>&>(t)))) {
   return cat::move(::cat::detail::get_aggregate_lvalue<in_index>(
      static_cast<remove_reference<S>&>(t)));
}

}  // namespace cat

namespace cat::detail {

// `make_index_sequence` over `cat::idx` to `cat::get` for
// `aggregate_apply_impl`
template <idx in_index, typename aggregate_type>
   requires(has_aggregate_get<remove_cvref<aggregate_type>>)
[[nodiscard, gnu::always_inline]]
constexpr auto
aggregate_get(aggregate_type&& t) -> decltype(auto) {
   return ::cat::get<in_index>(fwd(t));
}

template <typename callback_type, typename aggregate_type, idx... indices>
[[nodiscard, gnu::always_inline]]
constexpr auto
aggregate_apply_impl(callback_type&& callback, aggregate_type&& t,
                     index_list_type<indices...>)
   -> decltype(fwd(callback)(aggregate_get<indices>(fwd(t))...)) {
   return fwd(callback)(aggregate_get<indices>(fwd(t))...);
}

}  // namespace cat::detail

namespace cat {

// `apply` for `has_aggregate_get` types, separate overload from
// `is_tuple<>`-based `apply` in the main `tuple` header
template <typename callback_type, typename aggregate_type>
   requires(has_aggregate_get<remove_cvref<aggregate_type>>)
[[nodiscard]]
constexpr auto
apply(callback_type&& callback, aggregate_type&& t) {
   return detail::aggregate_apply_impl(
      fwd(callback), fwd(t),
      make_index_sequence<static_cast<idx>(__builtin_structured_binding_size(
         ::cat::remove_cvref<aggregate_type>))>{});
}

}  // namespace cat
