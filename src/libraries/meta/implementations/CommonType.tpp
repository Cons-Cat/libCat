// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {
namespace detail {
    template <typename... T>
    struct CommonTypeTrait;

    template <typename T>
    struct CommonTypeTrait<T> {
        using Type = T;
    };

    template <typename T, typename U>
    struct CommonTypeTrait<T, U> {
        // A ternary operator yields the most qualified type common to both
        // operands. If there is no common type, then the expression is
        // ill-formed.
        using Type = Decay<decltype(true ? declval<T>() : declval<U>())>;
    };

    template <typename T, typename U, typename... Remaining>
    struct CommonTypeTrait<T, U, Remaining...> {
        using Type =
            typename CommonTypeTrait<typename CommonTypeTrait<T, U>::Type,
                                     Remaining...>::Type;
    };
}  // namespace detail

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
using CommonType = typename detail::CommonTypeTrait<Ts...>::Type;

}  // namespace cat
