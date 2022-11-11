#include <cat/cast>

#include "../unit_tests.hpp"

struct TestCast1 {
    char storage[1];
};

struct TestCast2 {
    char storage[2];
};

struct TestCast4 {
    char storage[4];
};

struct TestCast8 {
    char storage[8];
};

TEST(test_cast) {
    static_assert(cat::is_same<decltype(cat::bit_int_cast(TestCast1{})), int1>);
    static_assert(cat::is_same<decltype(cat::bit_int_cast(TestCast2{})), int2>);
    static_assert(cat::is_same<decltype(cat::bit_int_cast(TestCast4{})), int4>);
    static_assert(cat::is_same<decltype(cat::bit_int_cast(TestCast8{})), int8>);
    // static_assert(cat::is_same<decltype(cat::reinterpret_int_cast(
    //                                (cat::AddConst<TestCast8>){})),
    //                            int8 const>);
}
