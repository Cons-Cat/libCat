auto main() -> int {
    // Test `concept`s.
    static_assert(cat::detail::ArithmeticNonPtr<int4>);
    static_assert(cat::detail::ArithmeticNonPtr<__INTPTR_TYPE__>);
    static_assert(!cat::detail::ArithmeticNonPtr<intptr<void>>);

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
};
