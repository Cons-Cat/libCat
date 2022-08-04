// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {
namespace detail {
    template <typename T, typename U>
    using ConditionalResolve =
        decltype(true ? declval<T (&)()>()() : declval<U (&)()>()());

    template <typename T, typename U>
    struct CommonReferenceDetailTrait {};

    template <typename T, typename U>
    using CommonReferenceDetail =
        typename CommonReferenceDetailTrait<T, U>::Type;

    template <typename T, typename U>
    using ConditionalResolveCvRef =
        ConditionalResolve<AddConstFrom<T, U>&, AddConstFrom<U, T>&>;

    template <typename T, typename U>
    using CommonReferenceDetailC =
        RemoveReference<CommonReferenceDetail<T&, U&>>&&;

    // If `T` and U are both rvalue-reference types:
    template <typename T, typename U>
        requires(is_rvalue_reference<T>, is_rvalue_reference<U>)
    struct CommonReferenceDetailTrait<T&&, U&&> {
        using Type = CommonReferenceDetailC<T, U>;
    };

    // (T const&, Y&)
    template <typename T, typename U>
    using CommonReferenceDetailD = CommonReferenceDetail<T const&, U&>;

    // If `T` is an rvalue-reference and `U` is an lvalue-reference:
    template <typename T, typename U>
        requires(is_rvalue_reference<T>&& is_lvalue_reference<U>)
    struct CommonReferenceDetailTrait<T&&, U&> {
        using Type = CommonReferenceDetailD<T, U>;
    };

    // If `T` is an lvalue-reference and `U` is an rvalue-reference:
    template <typename T, typename U>
        requires(is_lvalue_reference<T>&& is_rvalue_reference<U>)
    struct CommonReferenceDetailTrait<T&, U&&>
        : CommonReferenceDetailTrait<U&&, T&> {};

    template <typename T, typename U, template <typename> typename TQual,
              template <typename> typename UQual>
    struct BasicCommonReference {};

    template <typename T>
    struct AddConstRefQualifiers {
        template <typename U>
        using Type = AddConstFrom<T, U>;
    };

    template <typename T>
    struct AddConstRefQualifiers<T&> {
        template <typename U>
        using Type = AddConstFrom<T, U>&;
    };

    template <typename T>
    struct AddConstRefQualifiers<T&&> {
        template <typename U>
        using Type = AddConstFrom<T, U>&&;
    };

    template <typename T, typename U>
    using BasicCommonReferenceDetail = typename BasicCommonReference<
        RemoveCvRef<T>, RemoveCvRef<U>, AddConstRefQualifiers<T>::template Type,
        AddConstRefQualifiers<U>::template Type>::Type;

    // Recurse through template instantiations until one is well-formed.
    template <typename T, typename U, int recursion, typename = void>
    struct CommonReferenceRecursionTrait
        : CommonReferenceRecursionTrait<T, U, recursion + 1> {};

    // Both arguments are l-value references.
    template <typename T, typename U>
        requires(is_lvalue_reference<T>&& is_lvalue_reference<U>)
    struct CommonReferenceRecursionTrait<T&, U&, 0> {
        using Type = CommonReferenceDetail<T&, U&>;
    };

    // Both arguments are r-value references.
    template <typename T, typename U>
        requires(is_rvalue_reference<T>&& is_rvalue_reference<U>)
    struct CommonReferenceRecursionTrait<T&&, U&&, 0> {
        using Type = CommonReferenceDetail<T&&, U&&>;
    };

    // `T` is an l-value reference, and `U` is an r-value reference.
    template <typename T, typename U>
        requires(is_lvalue_reference<T>&& is_rvalue_reference<U>)
    struct CommonReferenceRecursionTrait<T&, U&&, 0> {
        using Type = CommonReferenceDetail<T&, U&&>;
    };

    // `T` is an r-value reference, and `U` is an l-value reference.
    template <typename T, typename U>
        requires(is_rvalue_reference<T>&& is_lvalue_reference<U>)
    struct CommonReferenceRecursionTrait<T&&, U&, 0> {
        using Type = CommonReferenceDetail<T&&, U&>;
    };

    // Neither `T` nor `U` are l-value or r-value references.
    template <typename T, typename U>
        requires(requires { detail::Void<BasicCommonReferenceDetail<T, U>>(); })
    struct CommonReferenceRecursionTrait<T, U, 1> {
        using Type = BasicCommonReferenceDetail<T, U>;
    };

    // Fall back to `ConditionalResolve`.
    template <typename T, typename U>
        requires(requires { detail::Void<ConditionalResolve<T, U>>(); })
    struct CommonReferenceRecursionTrait<
        T, U, 2, detail::Void<ConditionalResolve<T, U>>> {
        using Type = ConditionalResolve<T, U>;
    };

    // Fall back to `CommonType`.
    template <typename T, typename U>
        requires(requires { CommonType<T, U>(); })
    struct CommonReferenceRecursionTrait<T, U, 3> {
        using Type = CommonType<T, U>;
    };

    // Failing base-case.
    template <typename T, typename U>
    struct CommonReferenceRecursionTrait<T, U, 4> {};

    // template <typename...>
    // struct CommonTypePack {};

    // template <typename, typename, typename = void>
    // struct CommonTypeFold;

    // template <typename T, typename... Us>
    // struct CommonTypeFold<T, CommonTypePack<Us...>,
    //                       detail::Void<typename T::Type>>
    //     : CommonType<typename T::type, Us...> {};

    template <typename... T>
    struct CommonReferenceTrait;

    template <>
    struct CommonReferenceTrait<> {};

    // Single argument base-case.
    template <typename T>
    struct CommonReferenceTrait<T> {
        using Type = T;
    };

    // Multiple arguments base-case.
    template <typename T, typename U>
    struct CommonReferenceTrait<T, U> : CommonReferenceRecursionTrait<T, U, 0> {
    };

    // template <typename T, typename U, typename... Remaining>
    //     requires(sizeof...(Remaining) > 0)
    // struct CommonReferenceTrait<T, U, Remaining...>
    //     : CommonTypeFold<CommonReferenceTrait<T, U>,
    //                      CommonTypePack<Remaining...>> {};

    // template <typename T, typename U, typename... Remaining>
    //     requires(sizeof...(Remaining) > 0)
    // struct CommonTypeFold<CommonReferenceTrait<T, U>,
    //                       CommonTypePack<Remaining...>,
    //                       detail::Void<CommonReferenceTrait<T, U>>>
    //     : CommonReferenceTrait<CommonReferenceTrait<T, U>, Remaining...> {};
}  // namespace detail

template <typename... Ts>
    requires(sizeof...(Ts) > 0)
using CommonReference = typename detail::CommonReferenceTrait<Ts...>::Type;

}  // namespace cat
