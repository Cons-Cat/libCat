// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {
namespace detail {
    template <typename... T>
    struct common_type_trait;

    template <typename T>
    struct common_type_trait<T> {
        using type = T;
    };

    template <typename T, typename U>
    struct common_type_trait<T, U> {
        // A ternary operator yields the most qualified type common to both
        // operands. If there is no common type, then the expression is
        // ill-formed.
        using type = Decay<decltype(true ? declval<T>() : declval<U>())>;
    };

    template <typename T, typename U, typename... remaining>
    struct common_type_trait<T, U, remaining...> {
        using type =
            typename common_type_trait<typename common_type_trait<T, U>::type,
                                       remaining...>::type;
    };
}  // namespace detail

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
using common_type = typename detail::common_type_trait<Ts...>::type;

}  // namespace cat
