#include <cat/match>

#include "../unit_tests.hpp"
#include "cat/numerals"

template <auto value>
struct Nttp {
    static constexpr auto member = value;
};

TEST(test_numerals) {
    // Test `concept`s.
    static_assert(cat::detail::arithmeticNonPtr<int4>);
    static_assert(cat::detail::arithmeticNonPtr<__INTPTR_TYPE__>);
    static_assert(!cat::detail::arithmeticNonPtr<intptr<void>>);

    static_assert(cat::is_same<cat::ToUnsafeNumeral<int>, int>);
    static_assert(cat::is_same<cat::ToSafeNumeral<int>, int4>);
    static_assert(cat::is_same<cat::ToSafeNumeral<int4>, int4>);
    static_assert(cat::is_same<cat::ToUnsafeNumeral<int4>, int4::Raw>);

    // Test numerals' size,
    static_assert(sizeof(int1) == 1);
    static_assert(sizeof(uint1) == 1);
    static_assert(sizeof(int2) == 2);
    static_assert(sizeof(uint2) == 2);
    static_assert(sizeof(int4) == 4);
    static_assert(sizeof(uint4) == 4);
    static_assert(sizeof(int8) == 8);
    static_assert(sizeof(float4) == 4);
    static_assert(sizeof(float8) == 8);

    // Test that numerals are trivial.
    static_assert(cat::is_trivial<int1>);
    static_assert(cat::is_trivial<uint1>);
    static_assert(cat::is_trivial<int2>);
    static_assert(cat::is_trivial<uint2>);
    static_assert(cat::is_trivial<int4>);
    static_assert(cat::is_trivial<uint4>);
    static_assert(cat::is_trivial<int8>);
    static_assert(cat::is_trivial<uint8>);
    static_assert(cat::is_trivial<float4>);
    static_assert(cat::is_trivial<float8>);

    static_assert(cat::is_trivially_relocatable<int1>);
    static_assert(cat::is_trivially_relocatable<uint1>);
    static_assert(cat::is_trivially_relocatable<int2>);
    static_assert(cat::is_trivially_relocatable<uint2>);
    static_assert(cat::is_trivially_relocatable<int4>);
    static_assert(cat::is_trivially_relocatable<uint4>);
    static_assert(cat::is_trivially_relocatable<int8>);
    static_assert(cat::is_trivially_relocatable<uint8>);
    static_assert(cat::is_trivially_relocatable<float4>);
    static_assert(cat::is_trivially_relocatable<float8>);
    static_assert(cat::is_trivially_relocatable<intptr<void>>);
    static_assert(cat::is_trivially_relocatable<intptr<int>>);
    static_assert(cat::is_trivially_relocatable<uintptr<void>>);
    static_assert(cat::is_trivially_relocatable<uintptr<int>>);

    // Test `int4` constructors and assignment.
    int4 test_int4_1 = 1;
    int4 test_int4_2;
    test_int4_2 = 1;

    // Test `int4` operators.
    int4 int4_add = 1 + test_int4_1;
    int4_add = 1_i4 + test_int4_1;

    // `int4` pointer arithmetic.
    [[maybe_unused]] int* p_int4 = ((int*)(0)) + 1_i4;
    p_int4 = 1_i4 + ((int*)(0));

    // Test `intpr` constructors and assignment.
    intptr<void> intptr_1 = nullptr;
    intptr<void> intptr_2 = nullptr;
    intptr_1 = intptr_1 + intptr_2;

    // Test `<=>`.
    int4 int_less = 0;
    int4 int_more = 2;

    [[maybe_unused]] bool is_less = (int_less < int_more);
    is_less = ((0 <=> int_more) < 0);
    cat::verify(is_less);
    is_less = (0 < int_more);
    cat::verify(is_less);
    is_less = (int_less < 2);
    cat::verify(is_less);

    [[maybe_unused]] bool is_more = (int_more > int_less);
    is_more = ((0 <=> int_less) == 0);
    cat::verify(is_more);
    is_more = (0 < int_more);
    cat::verify(is_more);
    is_more = (int_less < 2);
    cat::verify(is_more);

    // Test generated `!=`.
    cat::verify(4_i4 == 4_i4);
    cat::verify(4_i4 != 2_i4);

    cat::verify(4_u4 == 4_u4);
    cat::verify(4_u4 != 2_u4);

    cat::verify(4_f4 == 4_f4);
    cat::verify(4_f4 != 2_f4);

    // Test matching numerals.
    int4 match_int = 1;
    bool matched = false;

    // Match type.
    cat::match(match_int)(  //
        is_a<uint4>().then([]() {
            cat::exit(1);
        }),
        is_a<int4>().then([&]() {
            matched = true;
        }));
    cat::verify(matched);

    // Match value.
    matched = false;
    cat::match(match_int)(  //
        is_a(0).then([]() {
            cat::exit(1);
        }),
        is_a(1).then([&]() {
            matched = true;
        }));
    cat::verify(matched);

    // Test unary operators.
    [[maybe_unused]] int4 negative_int4 = -1_i4;
    [[maybe_unused]] float4 negative_float4 = -1_f4;
    [[maybe_unused]] int4 positive_int4 = +1_i4;
    [[maybe_unused]] float4 positive_float4 = +1_f4;
    [[maybe_unused]] int4 negated_int4 = ~1_i4;

    // Test using numerals non-type template parameters.
    [[maybe_unused]] Nttp<1_i1> nttp_int1{};
    [[maybe_unused]] Nttp<1_u1> nttp_uint1{};
    [[maybe_unused]] Nttp<1_i2> nttp_int2{};
    [[maybe_unused]] Nttp<1_u2> nttp_uint2{};
    [[maybe_unused]] Nttp<1_i4> nttp_int4{};
    [[maybe_unused]] Nttp<1_u4> nttp_uint4{};
    [[maybe_unused]] Nttp<1_i8> nttp_int8{};
    [[maybe_unused]] Nttp<1_u8> nttp_uint8{};
    [[maybe_unused]] Nttp<1_f4> nttp_float4{};
    [[maybe_unused]] Nttp<1_f8> nttp_float8{};

    // Test unwrapping numerals in `NumericLimits`.
    static_assert(cat::NumericLimits<int4>::max() ==
                  cat::NumericLimits<int>::max());
    static_assert(cat::NumericLimits<float4>::max() ==
                  cat::NumericLimits<float>::max());
};
