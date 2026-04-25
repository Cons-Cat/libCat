// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/tuple>
#include <cat/type_list>

namespace cat {

namespace detail {
template <typename tuple_type, idx... indices>
constexpr auto
forward_tuple_impl(tuple_type&& input_tuple, index_list_type<indices...>) {
   using tuple_decay = decay<tuple_type>;
   using tuple_types = typename tuple_decay::types;
   return cat::tuple<typename tuple_types::template get<indices>...>{
      fwd(input_tuple).template get<indices>()...};
}

template <typename tuple_type>
constexpr auto
forward_tuple(tuple_type&& input_tuple) {
   using tuple_decay = decay<tuple_type>;
   return forward_tuple_impl(fwd(input_tuple),
                             make_index_sequence<tuple_decay::types::size>{});
}

template <typename left_tuple_type, typename right_tuple_type,
          idx... left_index, idx... right_index>
constexpr auto
tuple_cat_two_impl(left_tuple_type&& left, right_tuple_type&& right,
                   index_list_type<left_index...>,
                   index_list_type<right_index...>) {
   using left_decay = decay<left_tuple_type>;
   using right_decay = decay<right_tuple_type>;
   using left_types = typename left_decay::types;
   using right_types = typename right_decay::types;
   return tuple<typename left_types::template get<left_index>...,
                typename right_types::template get<right_index>...>{
      fwd(left).template get<left_index>()...,
      fwd(right).template get<right_index>()...};
}

template <typename left_tuple_type, typename right_tuple_type>
constexpr auto
tuple_cat_two(left_tuple_type&& left, right_tuple_type&& right) {
   using left_decay = decay<left_tuple_type>;
   using right_decay = decay<right_tuple_type>;
   return tuple_cat_two_impl(fwd(left), fwd(right),
                             make_index_sequence<left_decay::types::size>{},
                             make_index_sequence<right_decay::types::size>{});
}
}  // namespace detail

constexpr auto
tuple_cat() -> tuple<> {
   return {};
}

template <typename tuple_type>
   requires(is_tuple<tuple_type>)
constexpr auto
tuple_cat(tuple_type&& tuple) {
   return detail::forward_tuple(fwd(tuple));
}

template <typename left_tuple_type, typename right_tuple_type,
          typename... remaining_tuples>
   requires(is_tuple<left_tuple_type> && is_tuple<right_tuple_type>
            && (is_tuple<remaining_tuples> && ...))
constexpr auto
tuple_cat(left_tuple_type&& left, right_tuple_type&& right,
          remaining_tuples&&... remaining) {
   auto combined = detail::tuple_cat_two(fwd(left), fwd(right));
   if constexpr (sizeof...(remaining_tuples) == 0) {
      return combined;
   } else {
      return tuple_cat(move(combined), fwd(remaining)...);
   }
}
}  // namespace cat
