// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/tuple>
#include <cat/type_list>

namespace cat {

namespace detail {
    template <typename... OuterTupleElements>
    constexpr auto get_outer_type_list(TypeMap<OuterTupleElements...>) {
        return (TypeListFilled<Decay<OuterTupleElements>,
                               // `ssizeof_pack` causes an internal compiler
                               // error in GCC 12 here.
                               // ssizeof_pack<OuterTupleElements>()>
                               static_cast<ssize>(
                                   sizeof...(OuterTupleElements))>{} +
                ...);
    }

    template <typename... InnerTupleElements>
    constexpr auto get_inner_type_list(TypeMap<InnerTupleElements...>) {
        // Make a `TypeList` from every `TupleElement`, and concatenate the
        // lists together.
        return (TypeList<Decay<InnerTupleElements>>{} + ...);
    }

    // This takes a forwarding tuple as a parameter. The forwarding tuple only
    // contains references, so it should just be taken by value.
    template <typename OuterTuple, typename... OuterTupleElements,
              typename... InnerTupleElements>
    constexpr auto tuple_cat_detail(OuterTuple&& tuple,
                                    TypeList<OuterTupleElements...>,
                                    TypeList<InnerTupleElements...>)
        -> Tuple<typename InnerTupleElements::Element...> {
        return {// For every tuple passed in through `OuterTuple`, move the n'th
                // tuple's element storage into a new `Tuple` at the same index.
                move(tuple.template Identity<OuterTupleElements>::storage)
                    .template Identity<InnerTupleElements>::storage...};
    }
}  // namespace detail

template <typename... Ts>
constexpr auto tuple_cat(Ts&&... tuples) {
    if constexpr (sizeof...(Ts) == 0) {
        return Tuple<>{};
    } else {
        // Create a `Tuple` out of all the argument tuples.
        using OuterTuple = Tuple<Decay<Ts>...>;
        using OuterTupleTypeMap = typename OuterTuple::Map;

        // Get the `TypeList` from the inner and outer tuples.
        constexpr TypeList outer_elements_list =
            detail::get_outer_type_list(OuterTupleTypeMap{});
        constexpr TypeList inner_elements_list =
            detail::get_inner_type_list(OuterTupleTypeMap{});

        return detail::tuple_cat_detail(OuterTuple{move(tuples)...},
                                        outer_elements_list,
                                        inner_elements_list);
    }
}

}  // namespace cat
