#include <cat/tuple>

struct NonTrivial {
    NonTrivial(int){};
};

auto main() -> int {
    using intint = cat::Tuple<int, int>;
    static_assert(cat::is_trivial<intint>);
    static_assert(sizeof(intint) == 8);

    using non_and_int4 = cat::Tuple<NonTrivial, int4>;
    [[maybe_unused]] non_and_int4 test_intint4{1, int4{0}};
    static_assert(!cat::is_trivial<non_and_int4>);
    static_assert(sizeof(non_and_int4) == 4);

    // Test `Tuple` storage.
    intint tuple;
    int& left_1 = tuple.get<0>();
    left_1 = 10;
    int left_2 = tuple.get<0>();
    Result(left_2 == 10).or_exit();
    Result(left_2 == left_1).or_exit();
    Result(left_2 == tuple.first()).or_exit();
    tuple.second() = 20;

    // Test destructuring.
    auto& [int_1, int_2] = tuple;
    Result(int_1 == 10).or_exit();
    Result(int_2 == 20).or_exit();

    // Test aggregate construction.
    cat::Tuple<int, char> intchar = {100, 'a'};
    Result(intchar.first() == 100).or_exit();
    Result(intchar.second() == 'a').or_exit();

    // Test aggregate assignment.
    intchar = {200, 'b'};
    Result(intchar.first() == 200).or_exit();
    Result(intchar.second() == 'b').or_exit();

    // Test `const`.
    cat::Tuple<int, char> const intchar_const = {100, 'a'};
    Result(intchar_const.first() == 100).or_exit();
    Result(intchar_const.second() == 'a').or_exit();

    // Test move semantics.
    cat::Tuple<int, char>&& intchar_move = {100, 'a'};
    Result(cat::move(intchar_move.first()) == 100).or_exit();
    Result(cat::move(intchar_move.second()) == 'a').or_exit();

    cat::Tuple<int, char> const intchar_move_const = {100, 'a'};
    Result(cat::move(intchar_move_const.first()) == 100).or_exit();
    Result(cat::move(intchar_move_const.second()) == 'a').or_exit();

    // Test type deduction.
    cat::Tuple deduced = {0, 'b', 10.f};
    static_assert(cat::is_same<decltype(deduced.get<0>()), int&>);
    static_assert(cat::is_same<decltype(deduced.get<1>()), char&>);
    static_assert(cat::is_same<decltype(deduced.get<2>()), float&>);

    // Test `Tuple` auto-generated getters.
    cat::Tuple<char, int4, bool, void*, uint8> five_tuple;
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

    // Test that `Tuple` size is zero-overhead.
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

    // Test `Tuple` concatenation.
    cat::Tuple<int> concat_lhs = cat::Tuple<int>{10};
    cat::Tuple<float> concat_rhs = cat::Tuple<float>{1.f};
    cat::Tuple<int, float> concat_tuple = concat_lhs.concat(concat_rhs);
    Result(concat_tuple.first() == 10).or_exit();
    Result(concat_tuple.second() == 1.f).or_exit();

    // Test `Tuple` conversions.
    cat::Tuple<float, float> floatfloat = cat::Tuple<int, int>{10, 20};
    Result(floatfloat.first() == 10.f).or_exit();
    Result(floatfloat.second() == 20.f).or_exit();
}
