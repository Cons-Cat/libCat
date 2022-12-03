#include <cat/match>

#include "../unit_tests.hpp"

template <auto value>
struct Nttp {
    static constexpr auto member = value;
};

TEST(test_numerals) {
    // Test `concept`s.
    // static_assert(cat::detail::arithmeticNonPtr<int4>);
    // static_assert(cat::detail::arithmeticNonPtr<__INTPTR_TYPE__>);
    // static_assert(!cat::detail::arithmeticNonPtr<intptr<void>>);

    static_assert(cat::is_same<cat::ToRawArithmetic<int>, int>);
    static_assert(cat::is_same<cat::ToRawArithmetic<int4>, int4::Raw>);

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

    static_assert(cat::is_trivially_copyable<int1>);

    static_assert(cat::is_integral<int4>);
    static_assert(cat::is_integral<uint4>);
    static_assert(cat::is_integral<intptr<void>>);
    static_assert(cat::is_integral<uintptr<void>>);

    static_assert(cat::is_signed<int>);
    static_assert(cat::is_signed<float>);
    static_assert(cat::is_signed<float4>);
    static_assert(cat::is_signed<int4>);
    static_assert(!cat::is_signed<uint4>);

    static_assert(cat::is_unsigned<unsigned>);
    static_assert(!cat::is_unsigned<float>);
    static_assert(!cat::is_unsigned<float4>);
    static_assert(cat::is_unsigned<uint4>);
    static_assert(!cat::is_unsigned<int4>);

    static_assert(cat::is_signed_integral<int>);
    static_assert(!cat::is_signed_integral<unsigned>);
    static_assert(!cat::is_signed_integral<float>);

    static_assert(!cat::is_unsigned_integral<int>);
    static_assert(cat::is_unsigned_integral<unsigned>);
    static_assert(!cat::is_unsigned_integral<float>);

    // Test `IntFixed` and `UintFixed`.
    static_assert(cat::is_same<cat::IntFixed<1>, int1>);
    static_assert(cat::is_same<cat::IntFixed<2>, int2>);
    static_assert(cat::is_same<cat::IntFixed<4>, int4>);
    static_assert(cat::is_same<cat::IntFixed<8>, int8>);

    static_assert(cat::is_same<cat::UintFixed<1>, uint1>);
    static_assert(cat::is_same<cat::UintFixed<2>, uint2>);
    static_assert(cat::is_same<cat::UintFixed<4>, uint4>);
    static_assert(cat::is_same<cat::UintFixed<8>, uint8>);

    // Test `int4` constructors and assignment.
    int4 test_int4_1 = 1;
    int4 test_int4_2;
    test_int4_2 = 1;

    // Test `Arithmetic` operators.
    int4 int4_add = 1 + test_int4_1;
    int4_add = 1_i4 + test_int4_1;
    int4 int4_sub = 1 - test_int4_1;
    int4_sub = 1_i4 - test_int4_1;

    cat::verify(int4{1} == int4{1});

    // Greater than.
    cat::verify(int4{1} > int4{0});
    cat::verify(int4{1} >= int4{0});
    cat::verify(int4{1} >= int4{1});

    // Less than.
    cat::verify(int4{0} < int4{1});
    cat::verify(int4{0} <= int4{0});
    cat::verify(int4{0} <= int4{1});

    // Test `ArithmeticPtr` operators on raw numerals.
    // TODO: This has ambiguous overload resolution:
    // intptr<void> intptr_add_1 = 1 + intptr<void>{0};
    intptr<void> intptr_add_2 = intptr<void>{0} + 1;
    cat::verify(intptr_add_2 == 1);
    intptr<void> intptr_add_3 = 1_i4 + intptr<void>{0};
    cat::verify(intptr_add_3 == 1);
    intptr<void> intptr_add_4 = intptr<void>{0} + 1_i4;
    cat::verify(intptr_add_4 == 1);

    // TODO: This has ambiguous overload resolution:
    // intptr<void> intptr_sub_1 = 1 - intptr_add_2;
    intptr<void> intptr_sub_2 = intptr_add_2 - 1;
    cat::verify(intptr_sub_2 == 0);
    intptr<void> intptr_sub_3 = 1_i4 - intptr_add_2;
    cat::verify(intptr_sub_3 == 0);
    intptr<void> intptr_sub_4 = intptr_add_2 - 1_i4;
    cat::verify(intptr_sub_4 == 0);

    // Test `ArithmeticPtr` operators on safe `Arithmetic`s.
    intptr_add_2 = 1_i4 + intptr_add_2;
    intptr_add_2 = intptr_add_2 + 1_i4;
    intptr_sub_2 = 1_i4 - intptr_add_2;
    intptr_sub_2 = intptr_add_2 - 1_i4;

    // Test `ArithmeticPtr` operators on other `ArithmeticPtr`s.
    cat::verify((intptr<void>{0} + intptr<void>{1}) == 1);
    cat::verify((intptr<void>{0} - intptr<void>{1}) == -1);

    cat::verify(intptr<void>{1} == intptr<void>{1});

    // Greater than.
    cat::verify(intptr<void>{1} > intptr<void>{0});
    cat::verify(intptr<void>{1} >= intptr<void>{0});
    cat::verify(intptr<void>{1} >= intptr<void>{1});

    // Less than.
    cat::verify(intptr<void>{0} < intptr<void>{1});
    cat::verify(intptr<void>{0} <= intptr<void>{0});
    cat::verify(intptr<void>{0} <= intptr<void>{1});

    // TODO: Test integer promotion rules.
    static_assert(cat::is_same<decltype(intptr<void>{} + 1), intptr<void>>);
    static_assert(cat::is_same<decltype(intptr<void>{} - 1), intptr<void>>);

    // `int4` pointer arithmetic.
    char address;
    int* p_int4 = (reinterpret_cast<int*>(&address)) + 1_i4;
    p_int4 = 1_i4 + (reinterpret_cast<int*>(&address));
    p_int4 += 1_i4;
    p_int4 = p_int4 - 1_i4;
    p_int4 -= 1_i4;

    p_int4 += intptr<void>{0};
    p_int4 -= intptr<void>{0};
    p_int4 += uintptr<void>{0};
    p_int4 -= uintptr<void>{0};

    // Test `intpr` constructors and assignment.
    intptr<void> intptr_1 = nullptr;
    intptr<void> intptr_2 = nullptr;
    intptr_1 = intptr_1 + intptr_2;

    // Test `ArithmeticPtr` conversions.
    uintptr<void> uintptr_1 = static_cast<uintptr<void>>(intptr_1);
    [[maybe_unused]] uintptr<void>::Raw raw_uintptr =
        static_cast<uintptr<void>::Raw>(uintptr_1);
    [[maybe_unused]] intptr<void> intptr_3 =
        static_cast<intptr<void>>(uintptr_1);

    // Test `ArithmeticPtr` dereferencing operators.
    int4 integer = 0;
    intptr<int4> int_intptr = &integer;
    *int_intptr = 1;
    cat::verify(integer == 1);
    cat::verify(int_intptr->max == integer.max);  // NOLINT

    uint4 uinteger = 0u;
    intptr<uint4> uint_intptr = &uinteger;
    *uint_intptr = 1u;
    cat::verify(uinteger == 1u);
    cat::verify(uint_intptr->max == uinteger.max);  // NOLINT

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

    // Test `make_signed()` and `make_unsigned()`.
    static_assert(cat::is_same<decltype(cat::make_unsigned(1)), unsigned int>);
    static_assert(cat::is_same<decltype(cat::make_signed(1u)), int>);
    static_assert(cat::is_same<decltype(cat::make_unsigned(1_i4)), uint4>);
    static_assert(cat::is_same<decltype(cat::make_signed(1_u4)), int4>);

    static_assert(cat::make_sign_from<int4>(1u) == 1_i4);
    static_assert(cat::make_sign_from(2, 1u) == 1_i4);

    // Test unwrapped numerals in `Limits`.
    static_assert(cat::Limits<int4>::max() == cat::Limits<int4::Raw>::max());
    static_assert(cat::Limits<uint8>::max() == cat::Limits<uint8::Raw>::max());
    static_assert(cat::Limits<float4>::max() ==
                  cat::Limits<float4::Raw>::max());

    // Test unsigned saturating addition.
    static_assert(cat::sat_add(cat::uint1_max - 3u, 1_u1) < cat::uint1_max);
    static_assert(cat::sat_add(cat::uint1_max - 1u, 1_u1) == cat::uint1_max);
    static_assert(cat::sat_add(cat::uint1_max - 1_u1, 1_u1) == cat::uint1_max);
    static_assert(cat::sat_add(cat::uint1_max.raw, 1_u1) == cat::uint1_max);
    static_assert(cat::sat_add(cat::uint1_max, 100_u1) == cat::uint1_max);

    cat::verify(cat::sat_add(cat::uint1_max - 3u, 1_u1) < cat::uint1_max);
    cat::verify(cat::sat_add(cat::uint1_max - 1u, 1_u1) == cat::uint1_max);
    cat::verify(cat::sat_add(cat::uint1_max - 1u, 1u) == cat::uint1_max);
    cat::verify(cat::sat_add(cat::uint1_max.raw, 1_u1) == cat::uint1_max);
    cat::verify(cat::sat_add(cat::uint1_max, 100_u1) == cat::uint1_max);

    static_assert(cat::sat_add(cat::uint2_max - 3u, 2_u2) < cat::uint2_max);
    static_assert(cat::sat_add(cat::uint2_max - 2u, 2_u2) == cat::uint2_max);
    static_assert(cat::sat_add(cat::uint2_max - 1_u2, 2_u2) == cat::uint2_max);
    static_assert(cat::sat_add(cat::uint2_max.raw, 2_u2) == cat::uint2_max);
    static_assert(cat::sat_add(cat::uint2_max, 100_u2) == cat::uint2_max);

    cat::verify(cat::sat_add(cat::uint2_max - 3u, 2_u2) < cat::uint2_max);
    cat::verify(cat::sat_add(cat::uint2_max - 2u, 2_u2) == cat::uint2_max);
    cat::verify(cat::sat_add(cat::uint2_max - 1_u2, 2_u2) == cat::uint2_max);
    cat::verify(cat::sat_add(cat::uint2_max.raw, 2_u2) == cat::uint2_max);
    cat::verify(cat::sat_add(cat::uint2_max, 100_u2) == cat::uint2_max);

    static_assert(cat::sat_add(cat::uint4_max - 3u, 2_u4) < cat::uint4_max);
    static_assert(cat::sat_add(cat::uint4_max - 2u, 2_u4) == cat::uint4_max);
    static_assert(cat::sat_add(cat::uint4_max - 1u, 2u) == cat::uint4_max);
    static_assert(cat::sat_add(cat::uint4_max.raw, 2_u4) == cat::uint4_max);
    static_assert(cat::sat_add(cat::uint4_max, 100u) == cat::uint4_max);

    cat::verify(cat::sat_add(cat::uint4_max - 3u, 2_u4) < cat::uint4_max);
    cat::verify(cat::sat_add(cat::uint4_max - 2u, 2_u4) == cat::uint4_max);
    cat::verify(cat::sat_add(cat::uint4_max - 1u, 2u) == cat::uint4_max);
    cat::verify(cat::sat_add(cat::uint4_max.raw, 2_u4) == cat::uint4_max);
    cat::verify(cat::sat_add(cat::uint4_max, 100u) == cat::uint4_max);

    static_assert(cat::sat_add(cat::uint8_max - 3u, 2_u8) < cat::uint8_max);
    static_assert(cat::sat_add(cat::uint8_max - 2u, 2_u8) == cat::uint8_max);
    static_assert(cat::sat_add(cat::uint8_max - 1u, 2u) == cat::uint8_max);
    static_assert(cat::sat_add(cat::uint8_max.raw, 2_u8) == cat::uint8_max);
    static_assert(cat::sat_add(cat::uint8_max, 100u) == cat::uint8_max);

    cat::verify(cat::sat_add(cat::uint8_max - 3u, 2_u8) < cat::uint8_max);
    cat::verify(cat::sat_add(cat::uint8_max - 2u, 2_u8) == cat::uint8_max);
    cat::verify(cat::sat_add(cat::uint8_max - 1u, 2u) == cat::uint8_max);
    cat::verify(cat::sat_add(cat::uint8_max.raw, 2_u8) == cat::uint8_max);
    cat::verify(cat::sat_add(cat::uint8_max, 100u) == cat::uint8_max);

    // Test signed saturating addition.
    static_assert(cat::sat_add(cat::int1_max - 3_i1, 1_i1) < cat::int1_max);
    static_assert(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
    static_assert(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
    static_assert(cat::sat_add(cat::int1_max.raw, 1_i1) == cat::int1_max);
    static_assert(cat::sat_add(cat::int1_max, 100_i1) == cat::int1_max);

    cat::verify(cat::sat_add(cat::int1_max - 3, 1_i1) < cat::int1_max);
    cat::verify(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
    cat::verify(cat::sat_add(cat::int1_max - 1_i1, 1_i1) == cat::int1_max);
    cat::verify(cat::sat_add(cat::int1_max.raw, 1_i1) == cat::int1_max);
    cat::verify(cat::sat_add(cat::int1_max, 100_i1) == cat::int1_max);

    static_assert(cat::sat_add(cat::int2_max - 3_i2, 2_i2) < cat::int2_max);
    static_assert(cat::sat_add(cat::int2_max - 2_i2, 2_i2) == cat::int2_max);
    static_assert(cat::sat_add(cat::int2_max - 1_i2, 2_i2) == cat::int2_max);
    static_assert(cat::sat_add(cat::int2_max.raw, 2_i2) == cat::int2_max);
    static_assert(cat::sat_add(cat::int2_max, 100_i2) == cat::int2_max);

    cat::verify(cat::sat_add(cat::int2_max - 3_i2, 2_i2) < cat::int2_max);
    cat::verify(cat::sat_add(cat::int2_max - 2_i2, 2_i2) == cat::int2_max);
    cat::verify(cat::sat_add(cat::int2_max - 1_i2, 2_i2) == cat::int2_max);
    cat::verify(cat::sat_add(cat::int2_max.raw, 2_i2) == cat::int2_max);
    cat::verify(cat::sat_add(cat::int2_max, 100_i2) == cat::int2_max);

    static_assert(cat::sat_add(cat::int4_max - 3, 2_i4) < cat::int4_max);
    static_assert(cat::sat_add(cat::int4_max - 2, 2_i4) == cat::int4_max);
    static_assert(cat::sat_add(cat::int4_max - 1, 2) == cat::int4_max);
    static_assert(cat::sat_add(cat::int4_max.raw, 2_i4) == cat::int4_max);
    static_assert(cat::sat_add(cat::int4_max, 100) == cat::int4_max);

    cat::verify(cat::sat_add(cat::int4_max - 3, 2_i4) < cat::int4_max);
    cat::verify(cat::sat_add(cat::int4_max - 2, 2_i4) == cat::int4_max);
    cat::verify(cat::sat_add(cat::int4_max - 1, 2) == cat::int4_max);
    cat::verify(cat::sat_add(cat::int4_max.raw, 2_i4) == cat::int4_max);
    cat::verify(cat::sat_add(cat::int4_max, 100) == cat::int4_max);

    static_assert(cat::sat_add(cat::int8_max - 3, 2_i8) < cat::int8_max);
    static_assert(cat::sat_add(cat::int8_max - 2, 2_i8) == cat::int8_max);
    static_assert(cat::sat_add(cat::int8_max - 1, 2) == cat::int8_max);
    static_assert(cat::sat_add(cat::int8_max.raw, 2_i8) == cat::int8_max);
    static_assert(cat::sat_add(cat::int8_max, 100) == cat::int8_max);

    cat::verify(cat::sat_add(cat::int8_max - 3, 2_i8) < cat::int8_max);
    cat::verify(cat::sat_add(cat::int8_max - 2, 2_i8) == cat::int8_max);
    cat::verify(cat::sat_add(cat::int8_max - 1, 2) == cat::int8_max);
    cat::verify(cat::sat_add(cat::int8_max.raw, 2_i8) == cat::int8_max);
    cat::verify(cat::sat_add(cat::int8_max, 100) == cat::int8_max);

    // Test unsigned saturating subtraction.
    static_assert(cat::sat_sub(cat::uint1_min + 3u, 1_u1) > cat::uint1_min);
    static_assert(cat::sat_sub(cat::uint1_min + 1u, 1_u1) == cat::uint1_min);
    static_assert(cat::sat_sub(cat::uint1_min + 1_u1, 1_u1) == cat::uint1_min);
    static_assert(cat::sat_sub(cat::uint1_min.raw, 1_u1) == cat::uint1_min);
    static_assert(cat::sat_sub(cat::uint1_min, 100_u1) == cat::uint1_min);

    cat::verify(cat::sat_sub(cat::uint1_min + 3u, 1_u1) > cat::uint1_min);
    cat::verify(cat::sat_sub(cat::uint1_min + 1u, 1_u1) == cat::uint1_min);
    cat::verify(cat::sat_sub(cat::uint1_min + 1u, 1u) == cat::uint1_min);
    cat::verify(cat::sat_sub(cat::uint1_min.raw, 1_u1) == cat::uint1_min);
    cat::verify(cat::sat_sub(cat::uint1_min, 100_u1) == cat::uint1_min);

    static_assert(cat::sat_sub(cat::uint2_min + 3u, 2_u2) > cat::uint2_min);
    static_assert(cat::sat_sub(cat::uint2_min + 2u, 2_u2) == cat::uint2_min);
    static_assert(cat::sat_sub(cat::uint2_min + 1_u2, 2_u2) == cat::uint2_min);
    static_assert(cat::sat_sub(cat::uint2_min.raw, 2_u2) == cat::uint2_min);
    static_assert(cat::sat_sub(cat::uint2_min, 100_u2) == cat::uint2_min);

    cat::verify(cat::sat_sub(cat::uint2_min + 3u, 2_u2) > cat::uint2_min);
    cat::verify(cat::sat_sub(cat::uint2_min + 2u, 2_u2) == cat::uint2_min);
    cat::verify(cat::sat_sub(cat::uint2_min + 1_u2, 2_u2) == cat::uint2_min);
    cat::verify(cat::sat_sub(cat::uint2_min.raw, 2_u2) == cat::uint2_min);
    cat::verify(cat::sat_sub(cat::uint2_min, 100_u2) == cat::uint2_min);

    static_assert(cat::sat_sub(cat::uint4_min + 3u, 2_u4) > cat::uint4_min);
    static_assert(cat::sat_sub(cat::uint4_min + 2u, 2_u4) == cat::uint4_min);
    static_assert(cat::sat_sub(cat::uint4_min + 1u, 2u) == cat::uint4_min);
    static_assert(cat::sat_sub(cat::uint4_min.raw, 2_u4) == cat::uint4_min);
    static_assert(cat::sat_sub(cat::uint4_min, 100u) == cat::uint4_min);

    cat::verify(cat::sat_sub(cat::uint4_min + 3u, 2_u4) > cat::uint4_min);
    cat::verify(cat::sat_sub(cat::uint4_min + 2u, 2_u4) == cat::uint4_min);
    cat::verify(cat::sat_sub(cat::uint4_min + 1u, 2u) == cat::uint4_min);
    cat::verify(cat::sat_sub(cat::uint4_min.raw, 2_u4) == cat::uint4_min);
    cat::verify(cat::sat_sub(cat::uint4_min, 100u) == cat::uint4_min);

    static_assert(cat::sat_sub(cat::uint8_min + 3u, 2_u8) > cat::uint8_min);
    static_assert(cat::sat_sub(cat::uint8_min + 2u, 2_u8) == cat::uint8_min);
    static_assert(cat::sat_sub(cat::uint8_min + 1u, 2u) == cat::uint8_min);
    static_assert(cat::sat_sub(cat::uint8_min.raw, 2_u8) == cat::uint8_min);
    static_assert(cat::sat_sub(cat::uint8_min, 100u) == cat::uint8_min);

    cat::verify(cat::sat_sub(cat::uint8_min + 3u, 2_u8) > cat::uint8_min);
    cat::verify(cat::sat_sub(cat::uint8_min + 2u, 2_u8) == cat::uint8_min);
    cat::verify(cat::sat_sub(cat::uint8_min + 1u, 2u) == cat::uint8_min);
    cat::verify(cat::sat_sub(cat::uint8_min.raw, 2_u8) == cat::uint8_min);
    cat::verify(cat::sat_sub(cat::uint8_min, 100u) == cat::uint8_min);

    // Test signed saturating addition.
    static_assert(cat::sat_sub(cat::int1_min + 3_i1, 1_i1) > cat::int1_min);
    static_assert(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
    static_assert(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
    static_assert(cat::sat_sub(cat::int1_min.raw, 1_i1) == cat::int1_min);
    static_assert(cat::sat_sub(cat::int1_min, 100_i1) == cat::int1_min);

    cat::verify(cat::sat_sub(cat::int1_min + 3, 1_i1) > cat::int1_min);
    cat::verify(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
    cat::verify(cat::sat_sub(cat::int1_min + 1_i1, 1_i1) == cat::int1_min);
    cat::verify(cat::sat_sub(cat::int1_min.raw, 1_i1) == cat::int1_min);
    cat::verify(cat::sat_sub(cat::int1_min, 100_i1) == cat::int1_min);

    static_assert(cat::sat_sub(cat::int2_min + 3_i2, 2_i2) > cat::int2_min);
    static_assert(cat::sat_sub(cat::int2_min + 2_i2, 2_i2) == cat::int2_min);
    static_assert(cat::sat_sub(cat::int2_min + 1_i2, 2_i2) == cat::int2_min);
    static_assert(cat::sat_sub(cat::int2_min.raw, 2_i2) == cat::int2_min);
    static_assert(cat::sat_sub(cat::int2_min, 100_i2) == cat::int2_min);

    cat::verify(cat::sat_sub(cat::int2_min + 3_i2, 2_i2) > cat::int2_min);
    cat::verify(cat::sat_sub(cat::int2_min + 2_i2, 2_i2) == cat::int2_min);
    cat::verify(cat::sat_sub(cat::int2_min + 1_i2, 2_i2) == cat::int2_min);
    cat::verify(cat::sat_sub(cat::int2_min.raw, 2_i2) == cat::int2_min);
    cat::verify(cat::sat_sub(cat::int2_min, 100_i2) == cat::int2_min);

    static_assert(cat::sat_sub(cat::int4_min + 3, 2_i4) > cat::int4_min);
    static_assert(cat::sat_sub(cat::int4_min + 2, 2_i4) == cat::int4_min);
    static_assert(cat::sat_sub(cat::int4_min + 1, 2) == cat::int4_min);
    static_assert(cat::sat_sub(cat::int4_min.raw, 2_i4) == cat::int4_min);
    static_assert(cat::sat_sub(cat::int4_min, 100) == cat::int4_min);

    cat::verify(cat::sat_sub(cat::int4_min + 3, 2_i4) > cat::int4_min);
    cat::verify(cat::sat_sub(cat::int4_min + 2, 2_i4) == cat::int4_min);
    cat::verify(cat::sat_sub(cat::int4_min + 1, 2) == cat::int4_min);
    cat::verify(cat::sat_sub(cat::int4_min.raw, 2_i4) == cat::int4_min);
    cat::verify(cat::sat_sub(cat::int4_min, 100) == cat::int4_min);

    static_assert(cat::sat_sub(cat::int8_min + 3, 2_i8) > cat::int8_min);
    static_assert(cat::sat_sub(cat::int8_min + 2, 2_i8) == cat::int8_min);
    static_assert(cat::sat_sub(cat::int8_min + 1, 2) == cat::int8_min);
    static_assert(cat::sat_sub(cat::int8_min.raw, 2_i8) == cat::int8_min);
    static_assert(cat::sat_sub(cat::int8_min, 100) == cat::int8_min);

    cat::verify(cat::sat_sub(cat::int8_min + 3, 2_i8) > cat::int8_min);
    cat::verify(cat::sat_sub(cat::int8_min + 2, 2_i8) == cat::int8_min);
    cat::verify(cat::sat_sub(cat::int8_min + 1, 2) == cat::int8_min);
    cat::verify(cat::sat_sub(cat::int8_min.raw, 2_i8) == cat::int8_min);
    cat::verify(cat::sat_sub(cat::int8_min, 100) == cat::int8_min);

    // Test wrapping arithmetic operations.
    static_assert(cat::wrap_add(0, 1) == 1);
    static_assert(cat::wrap_add(cat::int4_max - 1, 1) == cat::int4_max);
    static_assert(cat::wrap_add(cat::int4_max, 1) == cat::int4_min);
    static_assert(cat::wrap_add(cat::int4_max, 2) == cat::int4_min + 1);

    static_assert(cat::wrap_add(0u, 1u) == 1u);
    static_assert(cat::wrap_add(cat::uint4_max - 1u, 1u) == cat::uint4_max);
    static_assert(cat::wrap_add(cat::uint4_max, 1u) == cat::uint4_min);
    static_assert(cat::wrap_add(cat::uint4_max, 2u) == cat::uint4_min + 1u);

    static_assert(cat::wrap_mul(0, 1) == 0);
    static_assert(cat::wrap_mul(cat::int4_max, 1) == cat::int4_max);
    static_assert(cat::wrap_mul(cat::int4_max, 2) == -2);

    static_assert(cat::wrap_mul(0u, 1u) == 0u);
    static_assert(cat::wrap_mul(cat::uint4_max, 1u) == cat::uint4_max);
    static_assert(cat::wrap_mul(cat::uint4_max, 2u) == cat::uint4_max - 1u);

    int4 safe_int = int4::max;
    cat::verify((safe_int.wrap + 100) == cat::int4_min + 99);

    safe_int.wrap += 1;
    cat::verify(safe_int == cat::int4_min);
    safe_int.wrap += 100;
    cat::verify(safe_int.wrap == cat::int4_min + 100);
    cat::verify(safe_int.sat == cat::int4_min + 100);
    cat::verify(safe_int.raw == cat::int4_min + 100);

    // Test saturating overflow with member access syntax.
    safe_int = int4::max;
    cat::verify((safe_int.sat + 100) == cat::int4_max);

    safe_int.sat += 1;
    cat::verify(safe_int == cat::int4_max);
    safe_int.sat += 100;
    cat::verify(safe_int.wrap == cat::int4_max);
    cat::verify(safe_int.sat == cat::int4_max);
    cat::verify(safe_int.raw == cat::int4_max);

    // Test overflow strong types.
    cat::wrap_int4 wrap_int4 = cat::int4_max;
    wrap_int4 += 100;
    cat::verify(wrap_int4 == cat::int4_min + 99);
    wrap_int4 = cat::int4_max;
    wrap_int4.sat += 100;
    cat::verify(wrap_int4 == cat::int4_max);

    cat::sat_int4 saturate_int4 = cat::int4_max;
    saturate_int4 += 100;
    cat::verify(saturate_int4 == cat::int4_max);
    saturate_int4 = cat::int4_max;
    saturate_int4.wrap += 100;
    cat::verify(saturate_int4 == cat::int4_min + 99);

    // Test floats.
    float4 safe_float = 0.f;
    safe_float = 2.f;
    cat::verify(safe_float == 2.f);
    safe_float.raw = 1.f;
    cat::verify(safe_float == 1.f);

    // Test bit-casts.
    cat::verify(__builtin_bit_cast(unsigned, 2_i4) == 2u);
};
