// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {
namespace detail {
    template <typename T, typename U>
    using ConditionallyResolveCommonReference =
        decltype(true ? declval<T (&)()>()() : declval<U (&)()>()());

    template <typename T, typename U, typename TQual = RemoveReference<T>,
              typename UQual = RemoveReference<U>>
    struct CommonReferenceDetailTrait;

    template <typename T, typename U>
    using CommonReferenceDetail =
        typename CommonReferenceDetailTrait<T, U>::Type;

    template <typename T, typename U>
    using CvCondRes = ConditionallyResolveCommonReference<CopyCvFrom<T, U>&,
                                                          CopyCvFrom<U, T>&>;

    template <typename T, typename U, typename TQual, typename UQual>
        requires(requires { typename CvCondRes<TQual, UQual>; } &&
                 is_reference<CvCondRes<TQual, UQual>>)
    struct CommonReferenceDetailTrait<T&, U&, TQual, UQual> {
        using Type = CvCondRes<TQual, UQual>;
    };

    template <typename T, typename U>
    using CommonReferenceDetailC =
        RemoveReference<CommonReferenceDetail<T&, U&>>&&;

    template <typename T, typename U, typename TQual, typename UQual>
        requires(requires { typename CommonReferenceDetailC<TQual, UQual>; } &&
                 is_convertible<T&&, CommonReferenceDetailC<TQual, UQual>> &&
                 is_convertible<U&&, CommonReferenceDetailC<TQual, UQual>>)
    struct CommonReferenceDetailTrait<T&&, U&&, TQual, UQual> {
        using Type = CommonReferenceDetailC<TQual, UQual>;
    };

    template <typename T, typename U>
    using CommonReferenceDetailD = CommonReferenceDetail<T const&, U&>;

    template <typename T, typename U, typename TQual, typename UQual>
        requires(requires { typename CommonReferenceDetailD<TQual, UQual>; } &&
                 is_convertible<T&&, CommonReferenceDetailD<TQual, UQual>>)
    struct CommonReferenceDetailTrait<T&&, U&, TQual, UQual> {
        using Type = CommonReferenceDetailD<TQual, UQual>;
    };

    template <typename T, typename U, typename TQual, typename UQual>
    struct CommonReferenceDetailTrait<T&, U&&, TQual, UQual>
        : CommonReferenceDetailTrait<U&&, T&> {};

    template <typename T, typename U, typename TQual, typename UQual>
    struct CommonReferenceDetailTrait {};

    template <typename T, typename U>
    struct CommonReferenceSubBullet3;
    template <typename T, typename U>
    struct CommonReferenceSubBullet2 : CommonReferenceSubBullet3<T, U> {};
    template <typename T, typename U>
    struct CommonReferenceSubBullet1 : CommonReferenceSubBullet2<T, U> {};

    // Attempt to resolve the common reference of `T` and `U` here, otherwise
    // fall back to `CommonReferenceSubBullet2`.
    template <typename T, typename U>
        requires(is_reference<T>&& is_reference<U>&& requires {
            typename CommonReferenceDetail<T, U>;
        })
    struct CommonReferenceSubBullet1<T, U> {
        using Type = CommonReferenceDetail<T, U>;
    };

    template <typename, typename, template <typename> typename,
              template <typename> typename>
    struct BasicCommonReferenceTrait {};

    // TODO: Can this be streamlined out?
    template <typename T>
    struct XRef {
        template <typename U>
        using Apply = CopyCvRefFrom<T, U>;
    };

    template <typename T, typename U>
    using BasicCommonReference =
        typename BasicCommonReferenceTrait<RemoveCvRef<T>, RemoveCvRef<U>,
                                           XRef<T>::template Apply,
                                           XRef<U>::template Apply>::Type;

    // Attempt to resolve the common reference of `T` and `U` here, otherwise
    // fall back to `CommonReferenceSubBullet3`.
    template <typename T, typename U>
        requires(requires { typename BasicCommonReference<T, U>; })
    struct CommonReferenceSubBullet2<T, U> {
        using Type = BasicCommonReference<T, U>;
    };

    // Attempt to resolve the common reference of `T` and `U` here, otherwise
    // fall back to `CommonType`.
    template <typename T, typename U>
        requires(requires {
            typename ConditionallyResolveCommonReference<T, U>;
        })
    struct CommonReferenceSubBullet3<T, U> {
        using Type = ConditionallyResolveCommonReference<T, U>;
    };

    // If no other templates satisfy `T` and `U`, then `CommonReference<T, U>`
    // is equivalent to `CommonType<T, U>`.
    template <typename T, typename U>
    struct CommonReferenceSubBullet3 : CommonType<T, U> {};

    template <typename...>
    struct CommonReferenceTrait;

    template <>
    struct CommonReferenceTrait<> {};

    template <typename T>
    struct CommonReferenceTrait<T> {
        using Type = T;
    };

    template <typename T, typename U>
    struct CommonReferenceTrait<T, U> : CommonReferenceSubBullet1<T, U> {};

    template <typename T, typename U, typename V, typename... Remaining>
        requires(requires { typename CommonReferenceTrait<T, U>; })
    struct CommonReferenceTrait<T, U, V, Remaining...>
        : CommonReferenceTrait<typename CommonReferenceTrait<T, U>::Type, V,
                               Remaining...> {};
}  // namespace detail

template <typename... Ts>
using CommonReference = typename detail::CommonReferenceTrait<Ts...>::Type;

}  // namespace cat
