#pragma once

#include <type_traits.h>
#include <utility.h>

/* To enhance legibility, I have prefixed type traits with std::, whereas
 * concepts have no prefix. */

// TODO: Remove usage of type_traits as much as possible.

template <typename T, typename U>
concept same_as = std::is_same_v<T, U>;

template <typename T>
concept integral = std::is_integral_v<T>;

template <typename T>
concept signed_integral = integral<T> && std::is_signed_v<T>;

template <typename T>
concept unsigned_integral = integral<T> && std::is_unsigned_v<T>;

template <typename T>
concept floating_point = !integral<T> && std::is_floating_point_v<T>;

template <typename T>
concept int_or_float = integral<T> || floating_point<T>;

// TODO: std::is_safe_integral<T> does not work.
template <typename T>
concept safe_integral =
    (!std::is_integral_v<T>)&&(!std::is_floating_point_v<T>);

template <typename Derived, typename Base>
concept derived_from = std::is_base_of_v<Base, Derived> &&
    std::is_convertible_v<const volatile Derived*, const volatile Base*>;

template <typename From, typename To>
concept convertible_to = std::is_convertible_v<From, To> && requires {
    static_cast<To>(std::declval<From>());
};

template <typename T, typename... Args>
concept constructible_from = std::is_constructible_v<T, Args...>;

template <typename T>
concept move_constructible = constructible_from<T, T> && convertible_to<T, T>;

template <typename T>
concept copy_constructible = move_constructible<T> &&
    constructible_from<T, T&> && convertible_to<T&, T> &&
    constructible_from<T, T const&> && convertible_to<T const&, T> &&
    constructible_from<T, T const> && convertible_to<T const, T>;

/* boolean_testable is adapted from the exposition-only concept
   boolean-testable. */
namespace detail {
    template <typename T>
    concept boolean_testable = convertible_to<T, bool>;
}

template <typename T>
concept boolean_testable = detail::boolean_testable<T> && requires(T&& b) {
    { !forward<T>(b) } -> detail::boolean_testable;
};

/* boolean_testable is adapted from the exposition-only concept
   __WeaklyEqualityComparableWith. */
namespace detail {
    template <typename T, typename U>
    concept weakly_equality_comparable_with =
        requires(std::remove_reference_t<T> const& t,
                 std::remove_reference_t<U> const& u) {
        { t == u } -> boolean_testable;
        { t != u } -> boolean_testable;
        { u == t } -> boolean_testable;
        { u != t } -> boolean_testable;
    };
}  // namespace detail

template <typename T>
concept equality_comparable = detail::weakly_equality_comparable_with<T, T>;

template <typename T>
concept enum_class = std::is_scoped_enum_v<T>;

/* Some concepts from the STL are not supported, for various reasons.
 * std::destructable is not useful without exception handling. */
