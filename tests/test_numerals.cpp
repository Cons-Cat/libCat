auto main() -> int {
    // Test `concept`s.
    static_assert(cat::detail::ArithmeticNonPtr<int4>);
    static_assert(!cat::detail::ArithmeticNonPtr<intptr<void>>);

    // Test `int4` constructors and assignment.
    int4 test_int4_1 = 1;
    int4 test_int4_2;
    test_int4_2 = 1;
    int4 int4_add = 1 + test_int4_1;
    int4_add = 1_i4 + test_int4_1;

    // Test `intpr` constructors and assignment.
    intptr<void> intptr_1 = nullptr;
    intptr<void> intptr_2 = nullptr;
    intptr_1 = intptr_1 + intptr_2;
};
