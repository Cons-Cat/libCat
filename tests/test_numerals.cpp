#include <cat/match>

template <auto value>
struct Nttp {
    static constexpr auto member = value;
};

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

    // Test `<=>`.
    int4 int_less = 0;
    int4 int_more = 2;

    [[maybe_unused]] bool is_less = (int_less < int_more);
    is_less = ((0 <=> int_more) < 0);
    Result(is_less).or_exit();
    is_less = (0 < int_more);
    Result(is_less).or_exit();
    is_less = (int_less < 2);
    Result(is_less).or_exit();

    [[maybe_unused]] bool is_more = (int_more > int_less);
    is_more = ((0 <=> int_less) == 0);
    Result(is_more).or_exit();
    is_more = (0 < int_more);
    Result(is_more).or_exit();
    is_more = (int_less < 2);
    Result(is_more).or_exit();

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
    Result(matched).or_exit();

    // Match value.
    matched = false;
    cat::match(match_int)(  //
        is_a(0).then([]() {
            cat::exit(1);
        }),
        is_a(1).then([&]() {
            matched = true;
        }));
    Result(matched).or_exit();

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
};
