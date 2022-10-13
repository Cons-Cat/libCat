#include <cat/compare>
#include <cat/tuple>

#include "../unit_tests.hpp"

struct Members {
    int member_variable;
    void member_function() {
    }
};

void test_is_function() {
}

template <typename T>
class TemplatedOne {};

template <typename T>
class TemplatedTwo {};

TEST(test_meta) {
    using namespace cat;
    static_assert(is_same<int, int>);
    static_assert(!is_same<int, unsigned int>);
    static_assert(sameAs<int, int>);
    static_assert(!sameAs<int, unsigned int>);

    enum Enum : int {
    };
    enum class EnumClass : int {
    };
    struct Struct {};
    class Class {};
    union Union {};

    static_assert(!is_enum<int>);
    static_assert(!is_enum<Struct>);
    static_assert(!is_enum<Class>);
    static_assert(!is_enum<Union>);
    static_assert(is_enum<Enum>);
    static_assert(is_enum<EnumClass>);

    static_assert(!is_scoped_enum<int>);
    static_assert(!is_scoped_enum<Struct>);
    static_assert(!is_scoped_enum<Class>);
    static_assert(!is_scoped_enum<Union>);
    static_assert(!is_scoped_enum<Enum>);
    static_assert(is_scoped_enum<EnumClass>);

    static_assert(!is_class<int>);
    static_assert(!is_class<Enum>);
    static_assert(!is_class<EnumClass>);
    static_assert(!is_class<Union>);
    static_assert(is_class<Struct>);
    static_assert(is_class<Class>);

    static_assert(is_union<Union>);
    static_assert(!is_union<int>);
    static_assert(!is_union<Enum>);
    static_assert(!is_union<EnumClass>);
    static_assert(!is_union<Struct>);
    static_assert(!is_union<Class>);

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
    static_assert(!is_reference<RemoveReference<int>>);
    static_assert(!is_reference<RemoveReference<int&>>);
    static_assert(!is_reference<RemoveReference<int&&>>);
    static_assert(!is_reference<RemoveReference<int const&>>);
    static_assert(!is_reference<RemoveReference<int const&&>>);

    // TODO: Test `Decay`.

    static_assert(is_reference<AddLvalueReference<int>>);
    static_assert(is_reference<AddLvalueReference<int&>>);
    static_assert(is_reference<AddLvalueReference<int&&>>);
    static_assert(is_reference<AddLvalueReference<int const&>>);
    static_assert(is_reference<AddLvalueReference<int const&&>>);

    static_assert(is_reference<AddRvalueReference<int>>);
    static_assert(is_reference<AddRvalueReference<int&>>);
    static_assert(is_reference<AddRvalueReference<int&&>>);

    static_assert(is_reference<AddRvalueReference<int>>);
    static_assert(is_reference<AddRvalueReference<int const&>>);
    static_assert(is_reference<AddRvalueReference<int const&&>>);

    static_assert(!is_lvalue_reference<int>);
    static_assert(is_lvalue_reference<int&>);
    static_assert(is_lvalue_reference<int const&>);
    static_assert(!is_lvalue_reference<int&&>);
    static_assert(!is_lvalue_reference<int const&&>);

    static_assert(is_lvalue_reference<AddLvalueReference<int>>);
    static_assert(is_lvalue_reference<AddLvalueReference<int&>>);
    static_assert(is_lvalue_reference<AddLvalueReference<int&&>>);

    static_assert(!is_rvalue_reference<int>);
    static_assert(!is_rvalue_reference<int&>);
    static_assert(is_rvalue_reference<int&&>);
    static_assert(is_rvalue_reference<int const&&>);

    static_assert(is_rvalue_reference<AddRvalueReference<int>>);
    // This is supposed to hold false, even with `std::add_rvalue_reference_t`:
    static_assert(!is_rvalue_reference<AddRvalueReference<int&>>);
    static_assert(is_rvalue_reference<AddRvalueReference<int&&>>);

    static_assert(is_const<int const>);
    static_assert(!is_const<int>);
    static_assert(!is_const<int const&>);  // This is correct.
    static_assert(!is_const<int const*>);

    static_assert(is_const<AddConst<int>>);
    static_assert(is_const<AddConst<int const>>);
    static_assert(!is_const<RemoveConst<int>>);
    static_assert(!is_const<RemoveConst<int const>>);

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
    static_assert(!is_signed<Class>);

    static_assert(is_unsigned<unsigned int>);
    static_assert(is_unsigned<Class>);
    static_assert(!is_unsigned<int>);
    static_assert(!is_unsigned<Signed>);

    static_assert(is_same<CopyConstFrom<int, int>, int>);
    static_assert(is_same<CopyConstFrom<int, int const>, int const>);
    static_assert(is_same<CopyConstFrom<int const, int>, int>);
    static_assert(is_same<CopyConstFrom<int, int const&>, int const>);
    static_assert(is_same<CopyConstFrom<int, int const&&>, int const>);
    static_assert(is_same<CopyConstFrom<int const, int&>, int>);
    static_assert(is_same<CopyConstFrom<int const, int&&>, int>);

    static_assert(is_same<CopyCvFrom<int, int>, int>);
    static_assert(is_same<CopyCvFrom<int const, int>, int>);
    static_assert(is_same<CopyCvFrom<int, int const>, int const>);
    static_assert(is_same<CopyCvFrom<int, int const&>, int const>);
    static_assert(is_same<CopyCvFrom<int, int const&&>, int const>);
    static_assert(is_same<CopyCvFrom<int const, int&>, int>);
    static_assert(is_same<CopyCvFrom<int const, int&&>, int>);

    static_assert(is_same<CopyRefFrom<int, int>, int>);
    static_assert(is_same<CopyRefFrom<int, int const>, int>);
    static_assert(is_same<CopyRefFrom<int, int const&>, int&>);
    static_assert(is_same<CopyRefFrom<int, int const&&>, int&&>);
    static_assert(is_same<CopyRefFrom<int const, int>, int const>);
    static_assert(is_same<CopyRefFrom<int const, int&>, int const&>);
    static_assert(is_same<CopyRefFrom<int const, int&&>, int const&&>);

    static_assert(is_same<CopyCvRefFrom<int, int>, int>);
    static_assert(is_same<CopyCvRefFrom<int, int const>, int const>);
    static_assert(is_same<CopyCvRefFrom<int, int const&>, int const&>);
    static_assert(is_same<CopyCvRefFrom<int, int const&&>, int const&&>);
    static_assert(is_same<CopyCvRefFrom<int const, int>, int>);
    static_assert(is_same<CopyCvRefFrom<int const, int&>, int&>);
    static_assert(is_same<CopyCvRefFrom<int const, int&&>, int&&>);
    // TODO:
    // static_assert(is_same<CopyCvRefFrom<int&, int const>, int const>);

    auto lambda = []() {
        return 0;
    };

    static_assert(!is_function<int>);
    static_assert(!is_function<int const>);
    static_assert(is_function<int(int)>);
    static_assert(is_function<decltype(test_is_function)>);
    static_assert(!is_function<decltype(lambda)>);

    static_assert(is_same<UnderlyingType<Enum>, int>);
    static_assert(is_same<UnderlyingType<EnumClass>, int>);

    static_assert(is_sizable<int>);
    static_assert(!is_sizable<void>);

    static_assert(is_same<CommonType<int, long int>, long int>);
    static_assert(is_same<CommonType<long int>, long int>);
    static_assert(is_same<CommonType<int, int>, int>);
    static_assert(is_same<CommonType<int const, int>, int>);
    static_assert(is_same<CommonType<int, long int, unsigned long int>,
                          unsigned long int>);

    static_assert(is_same<CommonReference<int, int>, int>);
    static_assert(is_same<CommonReference<int&, int&>, int&>);
    static_assert(is_same<CommonReference<int&&, int&&>, int&&>);
    static_assert(is_same<CommonReference<int, int&>, int>);
    static_assert(is_same<CommonReference<int&, int>, int>);
    static_assert(is_same<CommonReference<int&&, int>, int>);
    static_assert(is_same<CommonReference<int, int&&>, int>);

    static_assert(is_same<CommonReference<int const, int>, int>);
    static_assert(is_same<CommonReference<int const&, int&>, int const&>);
    static_assert(is_same<CommonReference<int const&&, int&&>, int const&&>);
    static_assert(is_same<CommonReference<int, int const>, int>);
    static_assert(is_same<CommonReference<int&, int const&>, int const&>);
    static_assert(is_same<CommonReference<int&&, int const&&>, int const&&>);

    static_assert(is_same<CommonReference<int&, int&&>, int const&>);
    static_assert(is_same<CommonReference<int const&&, int&>, int const&>);
    static_assert(is_same<CommonReference<int&, int const&&>, int const&>);
    static_assert(is_same<CommonReference<int const&, int&&>, int const&>);
    static_assert(is_same<CommonReference<int, int, int&>, int>);

    // TODO: This is supposed to work. It is blocked by the `Tuple` conversion
    // operator.
    // static_assert(
    //     is_same<CommonReference<Tuple<int, double>, Tuple<int&, double&>>,
    //             Tuple<int, double>>);
    // using TestTupleCommonRef = CommonReference<Tuple<int, double>,
    //     Tuple<int&, double&>>;

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
    static_assert(is_member_pointer<decltype(&Members::member_function)>);
    static_assert(
        is_member_function_pointer<decltype(&Members::member_function)>);
    static_assert(!is_member_function_pointer<decltype(&test_is_function)>);

    Members members;
    int Members::*p_member_variable = &Members::member_variable;
    members.*p_member_variable = 1;
    static_assert(is_member_pointer<decltype(p_member_variable)>);
    static_assert(is_member_object_pointer<decltype(p_member_variable)>);
    static_assert(is_member_object_pointer<int Members::*>);

    // Test `is_specialization`.
    static_assert(is_specialization<TemplatedOne<int>, TemplatedOne>);
    static_assert(!is_specialization<TemplatedOne<int>, TemplatedTwo>);
};
