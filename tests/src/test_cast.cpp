#include <cat/cast>

#include "../unit_tests.hpp"

struct test_cast1 {
    char storage[1];
};

struct test_cast2 {
    char storage[2];
};

struct test_cast4 {
    char storage[4];
};

struct test_cast8 {
    char storage[8];
};

TEST(test_cast) {
    static_assert(
        cat::is_same<decltype(cat::bit_int_cast(test_cast1{})), int1>);
    static_assert(
        cat::is_same<decltype(cat::bit_int_cast(test_cast2{})), int2>);
    static_assert(
        cat::is_same<decltype(cat::bit_int_cast(test_cast4{})), int4>);
    static_assert(
        cat::is_same<decltype(cat::bit_int_cast(test_cast8{})), int8>);
    // static_assert(cat::is_same<decltype(cat::reinterpret_int_cast(
    //                                (cat::add_const<test_cast8>){})),
    //                            int8 const>);
}
