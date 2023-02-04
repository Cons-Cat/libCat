// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat {
namespace detail {
    template <typename T, typename U>
    using conditionally_resolve_common_reference =
        decltype(true ? declval<T (&)()>()() : declval<U (&)()>()());

    template <typename T, typename U, typename TQual = remove_reference<T>,
              typename UQual = remove_reference<U>>
    struct common_reference_detail_trait;

    template <typename T, typename U>
    using common_reference_detail =
        typename common_reference_detail_trait<T, U>::type;

    template <typename T, typename U>
    using cv_conditional_resolve =
        conditionally_resolve_common_reference<copy_cv_from<T, U>&,
                                               copy_cv_from<U, T>&>;

    template <typename T, typename U, typename TQual, typename UQual>
        requires(requires { typename cv_conditional_resolve<TQual, UQual>; } &&
                 is_reference<cv_conditional_resolve<TQual, UQual>>)
    struct common_reference_detail_trait<T&, U&, TQual, UQual> {
        using type = cv_conditional_resolve<TQual, UQual>;
    };

    template <typename T, typename U>
    using common_reference_detail_c =
        remove_reference<common_reference_detail<T&, U&>>&&;

    template <typename T, typename U, typename TQual, typename UQual>
        requires(
            requires { typename common_reference_detail_c<TQual, UQual>; } &&
            is_convertible<T &&, common_reference_detail_c<TQual, UQual>> &&
            is_convertible<U &&, common_reference_detail_c<TQual, UQual>>)
    struct common_reference_detail_trait<T&&, U&&, TQual, UQual> {
        using type = common_reference_detail_c<TQual, UQual>;
    };

    template <typename T, typename U>
    using common_reference_detail_d = common_reference_detail<T const&, U&>;

    template <typename T, typename U, typename TQual, typename UQual>
        requires(
            requires { typename common_reference_detail_d<TQual, UQual>; } &&
            is_convertible<T &&, common_reference_detail_d<TQual, UQual>>)
    struct common_reference_detail_trait<T&&, U&, TQual, UQual> {
        using type = common_reference_detail_d<TQual, UQual>;
    };

    template <typename T, typename U, typename TQual, typename UQual>
    struct common_reference_detail_trait<T&, U&&, TQual, UQual>
        : common_reference_detail_trait<U&&, T&> {};

    template <typename T, typename U, typename TQual, typename UQual>
    struct common_reference_detail_trait {};

    template <typename T, typename U>
    struct common_reference_sub_bullet_3;

    template <typename T, typename U>
    struct common_reference_sub_bullet_2 : common_reference_sub_bullet_3<T, U> {
    };

    template <typename T, typename U>
    struct common_reference_sub_bullet_1 : common_reference_sub_bullet_2<T, U> {
    };

    // Attempt to resolve the common reference of `T` and `U` here, otherwise
    // fall back to `common_reference_sub_bullet_2`.
    template <typename T, typename U>
        requires(is_reference<T> && is_reference<U> &&
                 requires { typename common_reference_detail<T, U>; })
    struct common_reference_sub_bullet_1<T, U> {
        using type = common_reference_detail<T, U>;
    };

    template <typename, typename, template <typename> typename,
              template <typename> typename>
    struct basic_common_reference_trait {};

    // TODO: Can this be streamlined out?
    template <typename T>
    struct x_ref {
        template <typename U>
        using apply = copy_cvref_from<T, U>;
    };

    template <typename T, typename U>
    using basic_common_reference =
        typename basic_common_reference_trait<remove_cvref<T>, remove_cvref<U>,
                                              x_ref<T>::template apply,
                                              x_ref<U>::template apply>::type;

    // Attempt to resolve the common reference of `T` and `U` here, otherwise
    // fall back to `common_reference_sub_bullet_3`.
    template <typename T, typename U>
        requires(requires { typename basic_common_reference<T, U>; })
    struct common_reference_sub_bullet_2<T, U> {
        using type = basic_common_reference<T, U>;
    };

    // Attempt to resolve the common reference of `T` and `U` here, otherwise
    // fall back to `common_type`.
    template <typename T, typename U>
        requires(
            requires { typename conditionally_resolve_common_reference<T, U>; })
    struct common_reference_sub_bullet_3<T, U> {
        using type = conditionally_resolve_common_reference<T, U>;
    };

    // If no other templates satisfy `T` and `U`, then `common_reference<T, U>`
    // is equivalent to `common_type<T, U>`.
    template <typename T, typename U>
    struct common_reference_sub_bullet_3 : common_type<T, U> {};

    template <typename...>
    struct common_reference_trait;

    template <>
    struct common_reference_trait<> {};

    template <typename T>
    struct common_reference_trait<T> {
        using type = T;
    };

    template <typename T, typename U>
    struct common_reference_trait<T, U> : common_reference_sub_bullet_1<T, U> {
    };

    template <typename T, typename U, typename V, typename... remaining>
        requires(requires { typename common_reference_trait<T, U>; })
    struct common_reference_trait<T, U, V, remaining...>
        : common_reference_trait<typename common_reference_trait<T, U>::type, V,
                                 remaining...> {};
}  // namespace detail

template <typename... types>
using common_reference = detail::common_reference_trait<types...>::type;

}  // namespace cat
