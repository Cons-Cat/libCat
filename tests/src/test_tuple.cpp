#include <cat/tuple>

#include "../unit_tests.hpp"

struct TupNonTrivial {
    TupNonTrivial(int){};
};

TEST(test_tuple) {
    using intint = cat::tuple<int, int>;
    static_assert(cat::is_trivial<intint>);
    static_assert(sizeof(intint) == 8);

    using floatfloat = cat::tuple<float, float>;
    static_assert(cat::is_trivial<floatfloat>);
    static_assert(sizeof(floatfloat) == 8);

    using non_and_int4 = cat::tuple<TupNonTrivial, int4>;
    [[maybe_unused]] non_and_int4 test_intint4{1, int4{0}};
    static_assert(!cat::is_trivial<non_and_int4>);
    static_assert(sizeof(non_and_int4) == 4);

    // Test `tuple` storage.
    intint tuple;
    int& left_1 = tuple.get<0>();
    left_1 = 10;
    int left_2 = tuple.get<0>();
    cat::verify(left_2 == 10);
    cat::verify(left_2 == left_1);
    cat::verify(left_2 == tuple.first());
    tuple.second() = 20;

    // Test destructuring.
    auto& [int_1, int_2] = tuple;
    cat::verify(int_1 == 10);
    cat::verify(int_2 == 20);

    // Test aggregate construction.
    cat::tuple<int, char> intchar = {100, 'a'};
    cat::verify(intchar.first() == 100);
    cat::verify(intchar.second() == 'a');

    // Test aggregate assignment.
    intchar = {200, 'b'};
    cat::verify(intchar.first() == 200);
    cat::verify(intchar.second() == 'b');

    // Test `const`.
    cat::tuple<int, char> const intchar_const = {100, 'a'};
    cat::verify(intchar_const.first() == 100);
    cat::verify(intchar_const.second() == 'a');

    // Test move semantics.
    cat::tuple<int, char>&& intchar_move = {100, 'a'};
    cat::verify(cat::move(intchar_move.first()) == 100);
    cat::verify(cat::move(intchar_move.second()) == 'a');

    cat::tuple<int, char> const intchar_move_const = {100, 'a'};
    cat::verify(cat::move(intchar_move_const.first()) == 100);
    cat::verify(cat::move(intchar_move_const.second()) == 'a');

    // Test type deduction.
    cat::tuple deduced = {0, 'b', 10.f};
    static_assert(cat::is_same<decltype(deduced.get<0>()), int&>);
    static_assert(cat::is_same<decltype(deduced.get<1>()), char&>);
    static_assert(cat::is_same<decltype(deduced.get<2>()), float&>);

    // Test `tuple` auto-generated getters.
    cat::tuple<char, int4, bool, void*, uint8> five_tuple;
    static_assert(cat::is_same<decltype(five_tuple.first()), char&>);
    static_assert(cat::is_same<decltype(five_tuple.second()), int4&>);
    static_assert(cat::is_same<decltype(five_tuple.third()), bool&>);
    static_assert(cat::is_same<decltype(five_tuple.fourth()), void*&>);
    static_assert(cat::is_same<decltype(five_tuple.fifth()), uint8&>);

    // Test structured bindings.
    auto& [one, two, three, four, five] = five_tuple;
    one = 'a';
    two = 2;
    three = true;
    four = nullptr;
    five = 1u;

    // Test that `tuple` size is zero-overhead.
    static_assert(sizeof(intint) == sizeof(int) * 2);
    // This type is 32 bytes due to padding for member alignment.
    struct Five {
        // Eight bytes:
        char c;
        int4 i;
        // Eight bytes:
        bool b;
        // Sixteen bytes:
        void* p;
        uint8 u;
    };
    static_assert(sizeof(five_tuple) == sizeof(Five));

    // Test empty tuple.
    [[maybe_unused]] cat::tuple<> empty_tuple;
    [[maybe_unused]] cat::tuple<> empty_tuple_2{};

    // tuple of tuples.
    cat::tuple<cat::tuple<int, int>, cat::tuple<int, char>> tuple_of_tuple = {
        tuple, intchar};
    cat::tuple tuple_of_tuple_ctad = {tuple, intchar};
    static_assert(
        cat::is_same<decltype(tuple_of_tuple_ctad), decltype(tuple_of_tuple)>);

    cat::tuple_cat();
    // cat::tuple_cat(empty_tuple);
    // cat::tuple_cat(intint{0, 1}, floatfloat{2.f, 3.f});

    // // Test `tuple` concatenation.
    // cat::tuple<int> concat_lhs = cat::tuple<int>{10};
    // cat::tuple<float> concat_rhs = cat::tuple<float>{1.f};
    // cat::tuple<int, float> concat_tuple = concat_lhs.concat(concat_rhs);
    //     cat::verify(concat_tuple.first() == 10);
    //     cat::verify(concat_tuple.second() == 1.f);

    /*
        // Test `tuple` conversions.
        cat::tuple<float, float> floatfloat = cat::tuple<int, int>{10, 20};
            cat::verify(floatfloat.first() == 10.f);
            cat::verify(floatfloat.second() == 20.f);

        int l_int = 1;
        int r_int = 2;
        cat::tuple<int&, int&> tuple_ref_conv{l_int, r_int};
        cat::tuple<int, int> tuple_val = tuple_ref_conv;
        // cat::tuple<int const&, int const&> tuple_ref = tuple_val;
            */
}
