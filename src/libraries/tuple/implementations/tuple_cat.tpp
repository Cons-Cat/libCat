// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/tuple>
#include <cat/type_list>

namespace cat {

namespace detail {
    template <typename... outer_tuple_elements>
    constexpr auto get_outer_type_list(type_map<outer_tuple_elements...>) {
        return (type_list_filled<decay<outer_tuple_elements>,
                                 // `ssizeof_pack` causes an internal compiler
                                 // error in GCC 12 here.
                                 // ssizeof_pack<outer_tuple_elements>()>
                                 idx(sizeof...(outer_tuple_elements))>{} +
                ...);
    }

    template <typename... inner_tuple_elements>
    constexpr auto get_inner_type_list(type_map<inner_tuple_elements...>) {
        // Make a `type_list` from every `tuple_element`, and concatenate the
        // lists together.
        return (type_list<decay<inner_tuple_elements>>{} + ...);
    }

    // This takes a forwarding tuple as a parameter. The forwarding tuple only
    // contains references, so it should just be taken by value.
    template <typename outer_tuple, typename... outer_tuple_elements,
              typename... inner_tuple_elements>
    constexpr auto tuple_cat_detail(outer_tuple&& tuple,
                                    type_list<outer_tuple_elements...>,
                                    type_list<inner_tuple_elements...>)
        -> cat::tuple<typename inner_tuple_elements::element...> {
        return {
            // For every tuple passed in through `outer_tuple`, move the n'th
            // tuple's element storage into a new `tuple` at the same index.
            move(tuple.template Identity<outer_tuple_elements>::storage)
                .template Identity<inner_tuple_elements>::storage...};
    }
}  // namespace detail

template <typename... types>
constexpr auto tuple_cat(types&&... tuples) {
    if constexpr (sizeof...(types) == 0) {
        return tuple<>{};
    } else {
        // Create a `tuple` out of all the argument tuples.
        using outer_tuple = tuple<decay<types>...>;
        using outer_tuple_type_map = outer_tuple::Map;

        // Get the `type_list` from the inner and outer tuples.
        constexpr type_list outer_elements_list =
            detail::get_outer_type_list(outer_tuple_type_map());
        constexpr type_list inner_elements_list =
            detail::get_inner_type_list(outer_tuple_type_map());

        return detail::tuple_cat_detail(outer_tuple(move(tuples)...),
                                        outer_elements_list,
                                        inner_elements_list);
    }
}

}  // namespace cat
