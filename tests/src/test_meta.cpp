#include <cat/compare>
#include <cat/tuple>

#include "../unit_tests.hpp"

struct members {
    int member_variable;
    void member_function() {
    }
};

void test_is_function() {
}

template <typename T>
class templated_one {};

template <typename T>
class templated_two {};

TEST(test_meta) {
    using namespace cat;
    static_assert(is_same<int, int>);
    static_assert(!is_same<int, unsigned int>);

    enum enum_type : int
    {};
    enum class enum_class_type : int
    {};
    struct struct_type {};
    class class_type {};
    union union_type {};

    static_assert(!is_enum<int>);
    static_assert(!is_enum<struct_type>);
    static_assert(!is_enum<class_type>);
    static_assert(!is_enum<union_type>);
    static_assert(is_enum<enum_type>);
    static_assert(is_enum<enum_class_type>);

    static_assert(!is_scoped_enum<int>);
    static_assert(!is_scoped_enum<struct_type>);
    static_assert(!is_scoped_enum<class_type>);
    static_assert(!is_scoped_enum<union_type>);
    static_assert(!is_scoped_enum<enum_type>);
    // TODO: This stopped working. GCC 13 regression?
    // static_assert(is_scoped_enum<enum_class_type>);

    static_assert(!is_class<int>);
    static_assert(!is_class<enum_type>);
    static_assert(!is_class<enum_class_type>);
    static_assert(!is_class<union_type>);
    static_assert(is_class<struct_type>);
    static_assert(is_class<class_type>);

    static_assert(is_union<union_type>);
    static_assert(!is_union<int>);
    static_assert(!is_union<enum_type>);
    static_assert(!is_union<enum_class_type>);
    static_assert(!is_union<struct_type>);
    static_assert(!is_union<class_type>);

    static_assert(is_pointer<int*>);
    static_assert(is_pointer<int* const>);
    static_assert(!is_pointer<int>);
    static_assert(!is_pointer<int*&>);

    static_assert(is_reference<int&>);
    static_assert(is_reference<int const&>);
    static_assert(!is_reference<int>);

    static_assert(is_referenceable<int>);
    static_assert(is_referenceable<int&>);
    static_assert(is_referenceable<int&&>);
    static_assert(!is_referenceable<void>);

    static_assert(is_arithmetic<int>);
    static_assert(is_arithmetic<float>);
    static_assert(!is_arithmetic<void>);

    // TODO: Use `is_same`:
    static_assert(!is_reference<remove_reference<int>>);
    static_assert(!is_reference<remove_reference<int&>>);
    static_assert(!is_reference<remove_reference<int&&>>);
    static_assert(!is_reference<remove_reference<int const&>>);
    static_assert(!is_reference<remove_reference<int const&&>>);

    // TODO: Test `decay`.

    static_assert(is_reference<add_lvalue_reference<int>>);
    static_assert(is_reference<add_lvalue_reference<int&>>);
    static_assert(is_reference<add_lvalue_reference<int&&>>);
    static_assert(is_reference<add_lvalue_reference<int const&>>);
    static_assert(is_reference<add_lvalue_reference<int const&&>>);

    static_assert(is_reference<add_rvalue_reference<int>>);
    static_assert(is_reference<add_rvalue_reference<int&>>);
    static_assert(is_reference<add_rvalue_reference<int&&>>);

    static_assert(is_reference<add_rvalue_reference<int>>);
    static_assert(is_reference<add_rvalue_reference<int const&>>);
    static_assert(is_reference<add_rvalue_reference<int const&&>>);

    static_assert(!is_lvalue_reference<int>);
    static_assert(is_lvalue_reference<int&>);
    static_assert(is_lvalue_reference<int const&>);
    static_assert(!is_lvalue_reference<int&&>);
    static_assert(!is_lvalue_reference<int const&&>);

    static_assert(is_lvalue_reference<add_lvalue_reference<int>>);
    static_assert(is_lvalue_reference<add_lvalue_reference<int&>>);
    static_assert(is_lvalue_reference<add_lvalue_reference<int&&>>);

    static_assert(!is_rvalue_reference<int>);
    static_assert(!is_rvalue_reference<int&>);
    static_assert(is_rvalue_reference<int&&>);
    static_assert(is_rvalue_reference<int const&&>);

    static_assert(is_rvalue_reference<add_rvalue_reference<int>>);
    // This is supposed to hold false, even with `std::add_rvalue_reference_t`:
    static_assert(!is_rvalue_reference<add_rvalue_reference<int&>>);
    static_assert(is_rvalue_reference<add_rvalue_reference<int&&>>);

    static_assert(is_const<int const>);
    static_assert(!is_const<int>);
    static_assert(is_const<int const&>);
    static_assert(!is_const<int const*>);

    static_assert(is_const<add_const<int>>);
    static_assert(is_const<add_const<int const>>);
    static_assert(!is_const<remove_const<int>>);
    static_assert(!is_const<remove_const<int const>>);

    struct Signed {
        int data;
        constexpr Signed(int value) : data(value){};
        constexpr auto operator<(Signed other) const -> bool {
            return data < other.data;
        }
    };

    static_assert(is_signed<int>);
    static_assert(is_signed<Signed>);
    static_assert(!is_signed<unsigned int>);
    static_assert(!is_signed<class_type>);

    static_assert(is_unsigned<unsigned int>);
    static_assert(is_unsigned<class_type>);
    static_assert(!is_unsigned<int>);
    static_assert(!is_unsigned<Signed>);

    static_assert(is_same<add_const<int>, int const>);
    static_assert(is_same<add_const<int&>, int const&>);
    static_assert(is_same<add_const<int&&>, int const&&>);
    static_assert(is_same<add_const<int const>, int const>);
    static_assert(is_same<add_const<int const&>, int const&>);
    static_assert(is_same<add_const<int const&&>, int const&&>);

    static_assert(is_same<add_const<int volatile>, int const volatile>);
    static_assert(is_same<add_const<int volatile&>, int const volatile&>);
    static_assert(is_same<add_const<int volatile&&>, int const volatile&&>);
    static_assert(is_same<add_const<int const volatile>, int const volatile>);
    static_assert(is_same<add_const<int const volatile&>, int const volatile&>);
    static_assert(
        is_same<add_const<int const volatile&&>, int const volatile&&>);

    static_assert(is_same<add_volatile<int>, int volatile>);
    static_assert(is_same<add_volatile<int&>, int volatile&>);
    static_assert(is_same<add_volatile<int&&>, int volatile&&>);
    static_assert(is_same<add_volatile<int volatile>, int volatile>);
    static_assert(is_same<add_volatile<int volatile&>, int volatile&>);
    static_assert(is_same<add_volatile<int volatile&&>, int volatile&&>);

    static_assert(is_same<add_volatile<int const>, int const volatile>);
    static_assert(is_same<add_volatile<int const&>, int const volatile&>);
    static_assert(is_same<add_volatile<int const&&>, int const volatile&&>);
    static_assert(
        is_same<add_volatile<int const volatile>, int const volatile>);
    static_assert(
        is_same<add_volatile<int const volatile&>, int const volatile&>);
    static_assert(
        is_same<add_volatile<int const volatile&&>, int const volatile&&>);

    static_assert(is_same<copy_const_from<int, int>, int>);
    static_assert(is_same<copy_const_from<int const, int>, int const>);
    static_assert(is_same<copy_const_from<int, int const>, int>);
    static_assert(is_same<copy_const_from<int const&, int>, int const>);
    static_assert(is_same<copy_const_from<int const&&, int>, int const>);
    static_assert(is_same<copy_const_from<int&, int const>, int>);
    static_assert(is_same<copy_const_from<int&&, int const>, int>);

    static_assert(is_same<copy_cv_from<int, int>, int>);
    static_assert(is_same<copy_cv_from<int, int const>, int>);
    static_assert(is_same<copy_cv_from<int const, int>, int const>);
    static_assert(is_same<copy_cv_from<int const&, int>, int const>);
    static_assert(is_same<copy_cv_from<int const&&, int>, int const>);
    static_assert(is_same<copy_cv_from<int&, int const>, int>);
    static_assert(is_same<copy_cv_from<int&&, int const>, int>);

    static_assert(is_same<copy_ref_from<int, int>, int>);
    static_assert(is_same<copy_ref_from<int const, int>, int>);
    static_assert(is_same<copy_ref_from<int const&, int>, int&>);
    static_assert(is_same<copy_ref_from<int const&&, int>, int&&>);
    static_assert(is_same<copy_ref_from<int, int const>, int const>);
    static_assert(is_same<copy_ref_from<int&, int const>, int const&>);
    static_assert(is_same<copy_ref_from<int&&, int const>, int const&&>);

    static_assert(is_same<copy_cvref_from<int, int>, int>);
    static_assert(is_same<copy_cvref_from<int const, int>, int const>);
    static_assert(is_same<copy_cvref_from<int const&, int>, int const&>);
    static_assert(is_same<copy_cvref_from<int const&&, int>, int const&&>);
    static_assert(is_same<copy_cvref_from<int, int const>, int>);
    static_assert(is_same<copy_cvref_from<int&, int const>, int&>);
    static_assert(is_same<copy_cvref_from<int&&, int const>, int&&>);
    static_assert(is_same<copy_cvref_from<int const, int&>, int const>);

    auto lambda = []() {
        return 0;
    };

    static_assert(!is_function<int>);
    static_assert(!is_function<int const>);
    static_assert(is_function<int(int)>);
    static_assert(is_function<decltype(test_is_function)>);
    static_assert(!is_function<decltype(lambda)>);

    static_assert(is_same<underlying_type<enum_type>, int>);
    static_assert(is_same<underlying_type<enum_class_type>, int>);

    static_assert(is_sizable<int>);
    static_assert(!is_sizable<void>);

    static_assert(is_same<common_type<int, long int>, long int>);
    static_assert(is_same<common_type<long int>, long int>);
    static_assert(is_same<common_type<int, int>, int>);
    static_assert(is_same<common_type<int const, int>, int>);
    static_assert(is_same<common_type<int, long int, unsigned long int>,
                          unsigned long int>);

    static_assert(is_same<common_reference<int, int>, int>);
    static_assert(is_same<common_reference<int&, int&>, int&>);
    static_assert(is_same<common_reference<int&&, int&&>, int&&>);
    static_assert(is_same<common_reference<int, int&>, int>);
    static_assert(is_same<common_reference<int&, int>, int>);
    static_assert(is_same<common_reference<int&&, int>, int>);
    static_assert(is_same<common_reference<int, int&&>, int>);

    static_assert(is_same<common_reference<int const, int>, int>);
    static_assert(is_same<common_reference<int const&, int&>, int const&>);
    static_assert(is_same<common_reference<int const&&, int&&>, int const&&>);
    static_assert(is_same<common_reference<int, int const>, int>);
    static_assert(is_same<common_reference<int&, int const&>, int const&>);
    static_assert(is_same<common_reference<int&&, int const&&>, int const&&>);

    static_assert(is_same<common_reference<int&, int&&>, int const&>);
    static_assert(is_same<common_reference<int const&&, int&>, int const&>);
    static_assert(is_same<common_reference<int&, int const&&>, int const&>);
    static_assert(is_same<common_reference<int const&, int&&>, int const&>);
    static_assert(is_same<common_reference<int, int, int&>, int>);

    // TODO: This is supposed to work. It is blocked by the `tuple` conversion
    // operator.
    // static_assert(
    //     is_same<common_reference<tuple<int, double>, tuple<int&, double&>>,
    //             tuple<int, double>>);
    // using test_tuple_common_ref = common_reference<tuple<int, double>,
    //     tuple<int&, double&>>;

    static_assert(
        cat::is_same<std::common_comparison_category_t<std::strong_ordering,
                                                       std::weak_ordering>,
                     std::weak_ordering>);

    static_assert(
        cat::is_same<std::common_comparison_category_t<std::weak_ordering,
                                                       std::strong_ordering>,
                     std::weak_ordering>);

    static_assert(
        cat::is_same<std::common_comparison_category_t<std::partial_ordering,
                                                       std::weak_ordering>,
                     std::partial_ordering>);

    static_assert(
        cat::is_same<std::common_comparison_category_t<std::weak_ordering,
                                                       std::partial_ordering>,
                     std::partial_ordering>);

    static_assert(
        cat::is_same<std::common_comparison_category_t<std::partial_ordering,
                                                       std::strong_ordering>,
                     std::partial_ordering>);

    static_assert(
        cat::is_same<std::common_comparison_category_t<std::strong_ordering,
                                                       std::partial_ordering>,
                     std::partial_ordering>);

    // Test member type traits.
    static_assert(is_member_pointer<decltype(&members::member_function)>);
    static_assert(
        is_member_function_pointer<decltype(&members::member_function)>);
    static_assert(!is_member_function_pointer<decltype(&test_is_function)>);

    members members;
    int members::*p_member_variable = &members::member_variable;
    members.*p_member_variable = 1;
    static_assert(is_member_pointer<decltype(p_member_variable)>);
    static_assert(is_member_object_pointer<decltype(p_member_variable)>);
    static_assert(is_member_object_pointer<int members::*>);

    // Test `is_specialization`.
    static_assert(is_specialization<templated_one<int>, templated_one>);
    static_assert(!is_specialization<templated_one<int>, templated_two>);

    // Test signedness traits.
    static_assert(is_signed<make_signed_type<unsigned>>);
    static_assert(is_signed<make_signed_type<uint4>>);
    static_assert(is_signed<make_signed_type<float>>);
    static_assert(is_signed<make_signed_type<double>>);

    static_assert(!is_signed<make_unsigned_type<int>>);
    static_assert(!is_signed<make_unsigned_type<int4>>);

    static_assert(is_signed<copy_sign_from<int4, unsigned>>);
    static_assert(is_signed<copy_sign_from<int, uint4>>);
    static_assert(is_signed<copy_sign_from<int, unsigned>>);

    static_assert(!is_signed<copy_sign_from<uint4, int>>);
    static_assert(!is_signed<copy_sign_from<unsigned, int4>>);
    static_assert(!is_signed<copy_sign_from<unsigned, int>>);
};
