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

/*
template <typename...>
struct CommonType;

struct __do_common_type_impl {
    template <typename T, typename U>
    using __cond_t = decltype(true ? declval<T>() : declval<U>());

    // // if decay_t<decltype(false ? declval<D1>() : declval<D2>())>
    // // denotes a valid Type, let C denote that Type.
    // template <typename T, typename U>
    // static __success_type<Decay<__cond_t<T, U>>> _S_test(int);

    // // Otherwise, if COND-RES(CREF(D1), CREF(D2)) denotes a Type,
    // // let C denote the Type decay_t<COND-RES(CREF(D1), CREF(D2))>.
    // template <typename T, typename U>
    // static __success_type<RemoveCvRef<__cond_t<const T&, const U&>>>
    // _S_test_2(int);

    // template <typename, typename>
    // static __failure_type _S_test_2(...);

    // template <typename T, typename U>
    // static decltype(_S_test_2<T, U>(0)) _S_test(...);
};

// If sizeof...(T) is zero, there shall be no member Type.
template <>
struct CommonType<> {};

// If sizeof...(T) is one, the same Type, if any, as common_type_t<T0, T0>.
template <typename _Tp0>
struct CommonType<_Tp0> : public CommonType<_Tp0, _Tp0> {};

// If sizeof...(T) is two, ...
template <typename _Tp1, typename _Tp2, typename _Dp1 = Decay<_Tp1>,
          typename _Dp2 = Decay<_Tp2>>
struct __common_type_impl {
    // If is_same_v<T1, D1> is false or is_same_v<T2, D2> is false,
    // let C denote the same Type, if any, as common_type_t<D1, D2>.
    using Type = CommonType<_Dp1, _Dp2>;
};

template <typename _Tp1, typename _Tp2>
struct __common_type_impl<_Tp1, _Tp2, _Tp1, _Tp2>
    : private __do_common_type_impl {
    // Otherwise, if decay_t<decltype(false ? declval<D1>() : declval<D2>())>
    // denotes a valid Type, let C denote that Type.
    using Type = decltype(_S_test<_Tp1, _Tp2>(0));
};

// If sizeof...(T) is two, ...
template <typename _Tp1, typename _Tp2>
struct CommonType<_Tp1, _Tp2> : public __common_type_impl<_Tp1, _Tp2>::Type {};

template <typename...>
struct __common_type_pack {};

template <typename, typename, typename = void>
struct __common_type_fold;

// If sizeof...(T) is greater than two, ...
template <typename _Tp1, typename _Tp2, typename... _Rp>
struct CommonType<_Tp1, _Tp2, _Rp...>
    : public __common_type_fold<CommonType<_Tp1, _Tp2>,
                                __common_type_pack<_Rp...>> {};

// Let C denote the same Type, if any, as common_type_t<T1, T2>.
// If there is such a Type C, Type shall denote the same Type, if any,
// as common_type_t<C, R...>.
template <typename _CTp, typename... _Rp>
struct __common_type_fold<_CTp, __common_type_pack<_Rp...>,
                          detail::Void<typename _CTp::Type>>
    : public CommonType<typename _CTp::Type, _Rp...> {};

// Otherwise, there shall be no member Type.
template <typename _CTp, typename _Rp>
struct __common_type_fold<_CTp, _Rp, void> {};
*/
}  // namespace cat
