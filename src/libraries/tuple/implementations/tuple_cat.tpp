// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/tuple>
#include <cat/type_list>

namespace cat {

namespace detail {
template <typename Tuple, idx... indices>
constexpr auto
forward_tuple_impl(Tuple&& input_tuple, index_list_type<indices...>) {
   using tuple_decay = decay<Tuple>;
   using tuple_types = typename tuple_decay::types;
   return cat::tuple<typename tuple_types::template get<indices>...>{
      fwd(input_tuple).template get<indices>()...};
}

template <typename Tuple>
constexpr auto
forward_tuple(Tuple&& input_tuple) {
   using tuple_decay = decay<Tuple>;
   return forward_tuple_impl(fwd(input_tuple),
                             make_index_sequence<tuple_decay::types::size>{});
}

template <typename LeftTuple, typename RightTuple, idx... left_index,
          idx... right_index>
constexpr auto
tuple_cat_two_impl(LeftTuple&& left, RightTuple&& right,
                   index_list_type<left_index...>,
                   index_list_type<right_index...>) {
   using left_decay = decay<LeftTuple>;
   using right_decay = decay<RightTuple>;
   using left_types = typename left_decay::types;
   using right_types = typename right_decay::types;
   return tuple<typename left_types::template get<left_index>...,
                typename right_types::template get<right_index>...>{
      fwd(left).template get<left_index>()...,
      fwd(right).template get<right_index>()...};
}

template <typename LeftTuple, typename RightTuple>
constexpr auto
tuple_cat_two(LeftTuple&& left, RightTuple&& right) {
   using left_decay = decay<LeftTuple>;
   using right_decay = decay<RightTuple>;
   return tuple_cat_two_impl(fwd(left), fwd(right),
                             make_index_sequence<left_decay::types::size>{},
                             make_index_sequence<right_decay::types::size>{});
}
}  // namespace detail

constexpr auto
tuple_cat() -> tuple<> {
   return {};
}

template <typename Tuple>
   requires(is_tuple<Tuple>)
constexpr auto
tuple_cat(Tuple&& tuple) {
   return detail::forward_tuple(fwd(tuple));
}

template <typename LeftTuple, typename RightTuple, typename... Remaining>
   requires(is_tuple<LeftTuple> && is_tuple<RightTuple>
            && (is_tuple<Remaining> && ...))
constexpr auto
tuple_cat(LeftTuple&& left, RightTuple&& right, Remaining&&... remaining) {
   auto combined = detail::tuple_cat_two(fwd(left), fwd(right));
   if constexpr (sizeof...(Remaining) == 0) {
      return combined;
   } else {
      return tuple_cat(move(combined), fwd(remaining)...);
   }
}
}  // namespace cat
