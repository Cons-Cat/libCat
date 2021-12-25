#pragma once

/* Be warned, ye who trot here!
 * Here be arcane remnants of the dark arts.
 * Black magic befouls the lines between yon
 * copse, so lest your eyes be lead astray,
 * know that the incantations written yonder
 * hath driven many a pony to insanity.
 *
 * Enter at your peril! */

namespace meta {

// TODO: Assess if a type_traits library is really necessary.

namespace detail {
    template <typename T, typename U = T&&>
    auto declval(signed int) -> U;

    template <typename T>
    auto declval(signed long) -> T;
}  // namespace detail

template <typename T>
auto declval() -> decltype(detail::declval<T>(0));

template <typename T>
struct type_identity {
    using type = T;
};
template <typename T>
using type_identity_t = typename type_identity<T>::type;

template <typename...>
using void_t = void;

template <typename T>
struct remove_const {
    using type = T;
};
template <typename T>
using remove_const_t = typename remove_const<T>::type;

template <typename T>
struct remove_const<T const> {
    using type = T;
};

template <typename T>
struct remove_volatile {
    using type = T;
};
template <typename T>
struct remove_volatile<T volatile> {
    using type = T;
};

template <typename T>
struct remove_cv {
    using type = T;
};
template <typename T>
struct remove_cv<const T> {
    using type = T;
};
template <typename T>
struct remove_cv<volatile T> {
    using type = T;
};
template <typename T>
struct remove_cv<const volatile T> {
    using type = T;
};
template <typename T>
using remove_cv_t = typename remove_cv<T>::type;

template <typename T>
struct remove_reference {
    using type = T;
};
template <typename T>
struct remove_reference<T&> {
    using type = T;
};
template <typename T>
struct remove_reference<T&&> {
    using type = T;
};
template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

template <typename T>
struct remove_cvref {
    using type = remove_cv<remove_reference<T>>;
};
template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

namespace std::detail {
    template <class T>
    struct remove_pointer {
        using type = T;
    };
    template <class T>
    struct remove_pointer<T*> {
        using type = T;
    };
    template <class T>
    struct remove_pointer<T* const> {
        using type = T;
    };
    template <class T>
    struct remove_pointer<T* volatile> {
        using type = T;
    };
    template <class T>
    struct remove_pointer<T* const volatile> {
        using type = T;
    };
}  // namespace std::detail

template <typename T>
struct remove_pointer : std::detail::remove_pointer<T> {};
template <typename T>
using remove_pointer_t = typename remove_pointer<T>::type;

/* integral_constant has no constructor, because a `*_v` suffix contributes less
 * noise than parentheses or curly braces. */
template <typename T, T Value>
struct integral_constant {
    static constexpr T value = Value;
    using type = integral_constant<T, Value>;
};

// clang-tidy falsely warns about this.
template <typename T, T Value>                   // NOLINT
constexpr T integral_constant<T, Value>::value;  // NOLINT

template <bool Value>
using bool_constant = integral_constant<bool, Value>;

using true_type = bool_constant<true>;
constexpr bool true_type_v = true_type::value;

using false_type = bool_constant<false>;
constexpr bool false_type_v = false_type::value;

template <bool Condition, typename T, typename F>
struct conditional {
    using type = T;
};
template <typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};
template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// __is_enum() is a GNU builtin.
template <typename T>
struct is_enum : public integral_constant<bool, __is_enum(T)> {};
template <typename T>
constexpr bool is_enum_v = is_enum<T>::value;
template <typename T>
using is_enum_t = typename is_enum<T>::type;

// __is_same() is a GNU builtin.
template <typename T, typename U>
struct is_same : bool_constant<__is_same(T, U)> {};
template <typename T, typename U>
using is_same_t = typename is_same<T, U>::type;
template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

// __is_base_of() is a GNU builtin.
template <typename T, typename U>
struct is_base_of : bool_constant<__is_base_of(T, U)> {};
template <typename T, typename U>
constexpr bool is_base_of_v = is_base_of<T, U>::value;
template <typename T, typename U>
using is_base_of_t = typename is_base_of<T, U>::type;

// __is_union() is a GNU builtin.
template <typename T>
struct is_union : public integral_constant<bool, __is_union(T)> {};

// __is_class() is a GNU builtin.
template <typename T>
struct is_class : public integral_constant<bool, __is_class(T)> {};

/* is_literal_type was removed in C++17, but it was not replaced by a
 * better alternative. The standards committee decided it was generally
 * not useful, but I disagree. */
// __is_literal_type() is a GNU builtin.
template <typename T>
struct is_literal_type : public integral_constant<bool, __is_literal_type(T)> {
};
template <typename T>
constexpr bool is_literal_type_v = is_literal_type<T>::value;
template <typename T>
using is_literal_type_t = typename is_literal_type<T>::type;

namespace detail {
    // TODO: There has to be a more efficient way to exhaustively check
    // these.
    // template <typename>
    // struct is_safe_integral : false_type {};
    // template <>
    // struct is_safe_integral<i1> : true_type {};
    // template <>
    // struct is_safe_integral<safe_integral_t<unsigned char>> : true_type {};
    // template <>
    // struct is_safe_integral<safe_integral_t<signed short>> : true_type {};
    // template <>
    // struct is_safe_integral<safe_integral_t<unsigned short>> : true_type {};
    // template <>
    // struct is_safe_integral<safe_integral_t<signed int>> : true_type {};
    // template <>
    // struct is_safe_integral<safe_integral_t<unsigned int>> : true_type {};
    // template <>
    // struct is_safe_integral<safe_integral_t<signed long>> : true_type {};
    // template <>
    // struct is_safe_integral<safe_integral_t<unsigned long>> : true_type {};
    // // TODO: This makes an error, and I'm not sure why:
    // // template <>
    // // struct is_safe_integral<i128> : true_type {};
    // // template <>
    // // struct is_safe_integral<u128> : true_type {};

    /* I tried using the fixed-width types initially, but some of the templates
     * would fail to resolve, for some reason. */
    template <typename>
    struct is_integral : false_type {};
    template <>
    struct is_integral<signed char> : true_type {};
    template <>
    struct is_integral<unsigned char> : true_type {};
    template <>
    struct is_integral<signed short> : true_type {};
    template <>
    struct is_integral<unsigned short> : true_type {};
    template <>
    struct is_integral<signed int> : true_type {};
    template <>
    struct is_integral<unsigned int> : true_type {};
    template <>
    struct is_integral<signed long> : true_type {};
    template <>
    struct is_integral<unsigned long> : true_type {};
    template <>
    struct is_integral<bool> : true_type {};
}  // namespace detail

// template <typename T>
// struct is_safe_integral : detail::is_safe_integral<remove_cv_t<T>> {};
// template <typename T>
// constexpr bool is_safe_integral_v = is_safe_integral<T>::value;
// template <typename T>
// using is_safe_integral_t = typename is_safe_integral<T>::type;

template <typename T>
struct is_integral : detail::is_integral<remove_cv_t<T>> {};
// is_integral_v<> does not work, and I am not sure why.
template <typename T>
constexpr bool is_integral_v = is_integral<T>::value;
template <typename T>
using is_integral_t = typename is_integral<T>::type;

template <typename T>
struct is_floating_point
    : bool_constant<is_same_v<float, remove_cv_t<T>> ||
                    is_same_v<double, remove_cv_t<T>> ||
                    is_same_v<long double, remove_cv_t<T>>> {};

template <typename T>
constexpr bool is_floating_point_v = is_floating_point<T>::value;
template <typename T>
using is_floating_point_t = typename is_floating_point<T>::type;

template <typename T>
struct is_arithmetic : conditional<is_integral_v<T>, is_integral<T>,
                                   is_floating_point<T>>::type {};

namespace detail {
    template <typename T, bool = is_arithmetic<T>::value>
    struct is_signed : bool_constant<T(-1) < T(0)> {};

    template <typename T>
    struct is_signed<T, false> : false_type {};
}  // namespace detail

template <typename T>
struct is_signed : detail::is_signed<T>::type {};
template <typename T>
constexpr bool is_signed_v = is_signed<T>::value;
template <typename T>
using is_signed_t = typename is_signed<T>::type;

namespace detail {
    template <typename T, bool = is_arithmetic<T>::value>
    struct is_unsigned : bool_constant<T(0) < T(-1)> {};
    template <typename T>
    struct is_unsigned<T, false> : false_type {};
}  // namespace detail

template <typename T>
struct is_unsigned : detail::is_unsigned<T>::type {};
template <typename T>
constexpr bool is_unsigned_v = is_unsigned<T>::value;
template <typename T>
using is_unsigned_t = typename is_unsigned<T>::type;

template <typename T>
struct is_void : is_same<void, typename remove_cv<T>::type> {};
template <typename T>
constexpr bool is_void_v = is_void<T>::value;

template <typename>
struct is_const : false_type {};
template <typename T>
struct is_const<T const> : true_type {};
template <typename T>
constexpr bool is_const_v = is_const<T>::value;
template <typename T>
using is_const_t = typename is_const<T>::type;

template <typename>
struct is_volatile : false_type {};
template <typename T>
struct is_volatile<T volatile> : true_type {};

namespace detail {
    template <typename T>
    struct is_pointer : false_type {};
    template <typename T>
    struct is_pointer<T*> : true_type {};
}  // namespace detail

template <typename T>
struct is_pointer : detail::is_pointer<T> {};
template <typename T>
constexpr bool is_pointer_v = is_pointer<T>::value;
template <typename T>
using is_pointer_t = typename is_pointer<T>::type;

template <typename>
struct is_array : false_type {};
template <typename T, int Size>
struct is_array<T[Size]> : true_type {};
template <typename T>
struct is_array<T[]> : true_type {};
template <typename T>
constexpr bool is_array_v = is_array<T>::value;
template <typename T>
using is_array_t = typename is_array<T>::type;

template <typename T>
struct remove_extent {
    using type = T;
};

template <typename T, int Size>
struct remove_extent<T[Size]> {
    using type = T;
};

template <typename T>
struct remove_extent<T[]> {
    using type = T;
};

template <typename T>
struct is_function : public bool_constant<!is_const<const T>::value> {};

template <typename T>
struct is_function<T&> : false_type {};

template <typename T>
struct is_function<T&&> : false_type {};

template <typename T, typename = void>
struct is_referenceable : false_type {};

template <typename T>
struct is_referenceable<T, void_t<T&>> : true_type {};

namespace detail {
    template <typename T,
              bool = conditional<is_referenceable<T>::value,
                                 is_referenceable<T>, is_void<T>>::value>
    struct add_pointer {
        using type = T;
    };

    template <typename T>
    struct add_pointer<T, true> {
        using type = remove_reference_t<T>*;
    };
}  // namespace detail

template <typename T>
struct add_pointer : detail::add_pointer<T> {};

template <typename T>
struct decay {
  private:
    using U = typename remove_reference<T>::type;

  public:
    using type = typename conditional<
        is_array<U>::value, typename remove_extent<U>::type*,
        typename conditional<is_function<U>::value,
                             typename add_pointer<U>::type,
                             typename remove_cv<U>::type>::type>::type;
};
template <typename T>
using decay_t = typename decay<T>::type;

template <typename T>
struct underlying_type {
    using type = __underlying_type(T);
};
template <typename T>
using underlying_type_t = typename underlying_type<T>::type;

namespace detail {
    template <
        typename From, typename To,
        bool = conditional<is_void<From>::value, is_void<From>,
                           conditional<is_function<To>::value, is_function<To>,
                                       is_array<To>>>::value>
    struct is_convertible {
        using type = typename is_void<To>::type;
    };
}  // namespace detail

template <typename From, typename To>
struct is_convertible : detail::is_convertible<From, To>::type {};
template <typename From, typename To>
using is_convertible_t = typename is_convertible<From, To>::type;
template <typename From, typename To>
constexpr bool is_convertible_v = is_convertible<From, To>::value;

// is_scoped_enum is a C++23 type_trait.
namespace detail {
    template <typename T, bool = is_enum_v<T>>
    struct is_scoped_enum : false_type {};
    template <typename T>
    struct is_scoped_enum<T, true>
        : bool_constant<!is_convertible_v<T, underlying_type_t<T>>> {};
}  // namespace detail

template <typename T>
struct is_scoped_enum : detail::is_scoped_enum<T> {};
template <typename T>
constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;
template <typename T>
using is_scoped_enum_t = typename is_scoped_enum<T>::type;

template <typename>
struct is_lvalue_reference : false_type {};
template <typename T>
struct is_lvalue_reference<T&> : true_type {};
template <typename T>
constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;
template <typename T>
using is_lvalue_reference_t = typename is_lvalue_reference<T>::type;

template <typename>
struct is_rvalue_reference : false_type {};
template <typename T>
struct is_rvalue_reference<T&&> : true_type {};
template <typename T>
constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;
template <typename T>
using is_rvalue_reference_t = typename is_rvalue_reference<T>::type;

template <typename T, unsigned long = sizeof(T)>
constexpr auto is_complete_or_unbounded(type_identity<T>) -> true_type {
    return {};
}

// __is_constructible is a GCC built-in.
namespace detail {
    template <typename T, typename... Args>
    struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};
}  // namespace detail

template <typename T, typename... Args>
struct is_constructible : detail::is_constructible<T, Args...> {
    static_assert(is_complete_or_unbounded(type_identity<T>{}));
};
template <typename T, typename... Args>
constexpr bool is_constructible_v = is_constructible<T, Args...>::value;
template <typename T, typename... Args>
using is_constructible_t = typename is_constructible<T, Args...>::type;

template <typename T>
struct is_default_constructible : detail::is_constructible<T>::type {
    static_assert(is_complete_or_unbounded(type_identity<T>{}));
};
template <typename T, typename>
constexpr bool is_default_constructible_v = is_default_constructible<T>::value;
template <typename T, typename>
using is_default_constructible_t = typename is_default_constructible<T>::type;

// https://stackoverflow.com/a/31763111
template <typename T, template <typename...> typename Template>
struct is_specialization : false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : true_type {};

}  // namespace meta
