#include <cat/variant>

auto main() -> int {
    cat::Variant<int, char, uint4> variant(int{1});
    verify(variant.is<int>());
    verify(variant.holds_alternative<int>());
    int foo_int = variant.get<int>();
    verify(foo_int == 1);

    static_assert(variant.alternative_index<int> == 0);
    static_assert(variant.alternative_index<char> == 1);
    static_assert(variant.alternative_index<uint4> == 2);

    variant = 'o';
    verify(variant.is<char>());
    verify(variant.holds_alternative<char>());
    char foo_char = variant.get<char>();
    verify(foo_char == 'o');

    cat::Optional<char&> opt1 = variant.get_if<char>();
    verify(opt1.has_value());
    cat::Optional opt2 = variant.get_if<int>();
    verify(!opt2.has_value());

    // Test variant subtype constructor and assignment operator.
    cat::Variant<int, char, uint4, int2> variant2 = variant;
    verify(variant2.is<char>());
    verify(variant2.holds_alternative<char>());
    variant2 = 1;
    verify(variant2.is<int>());
    verify(variant2.holds_alternative<int>());
    variant2 = variant;
    verify(variant2.is<char>());
    verify(variant2.holds_alternative<char>());

    variant = 1;
    cat::Variant<int, char, uint4, int2> variant3 = variant;
    verify(variant3.is<int>());
    verify(variant3.holds_alternative<int>());
    variant3 = int2{10};
    verify(variant3.is<int2>());
    verify(variant3.holds_alternative<int2>());
    variant3 = variant;
    verify(variant3.is<int>());
    verify(variant3.holds_alternative<int>());

    // Test getting variant type by index.
    static_assert(cat::is_same<decltype(variant3.get<0>()), int>);
    static_assert(cat::is_same<decltype(variant3.get<1>()), char>);
    static_assert(cat::is_same<decltype(variant3.get<2>()), uint4>);
    static_assert(cat::is_same<decltype(variant3.get<3>()), int2>);

    // Test constant-evaluating `Variant`.
    constexpr cat::Variant<int, uint4> const_variant = 1;
    static_assert(const_variant.get<int>() == 1);

    // Test `.is()`.
    variant3 = int{1};
    verify(variant3.is<int>());
    verify(variant3.is(1));

    variant3 = 'b';
    verify(variant3.is<char>());
    verify(variant3.is('b'));

    // `.is()` accepts unconditionally invalid types, unlike
    // `.holds_alternative()`.
    verify(!variant3.is<unsigned long long>());

    // Test pattern matching.
    variant3 = int{1};
    bool matched = false;
    cat::match(variant3)(  //
        one_of<double, float>().then([]() {
            // This should never match.
            cat::exit(1);
        }),
        one_of<float, int>().then([&]() {
            // This should match, because it is an `int`.
            matched = true;
        }));
    verify(matched);

    matched = false;
    cat::match(variant3)(  //
        is_a<float>().then([&]() {
            cat::exit(1);
        }),
        is_a<int>().then([&]() {
            matched = true;
        }));
    verify(matched);

    // `variant3` holds an integer, but floats are convertible to integers.
    matched = false;
    cat::match(variant3)(  //
        is_a(2.f).then([&]() {
            cat::exit(1);
        }),
        is_a(1.f).then([&]() {
            matched = true;
        }));
    verify(matched);

    // Test member access pattern matching syntax.
    matched = false;
    variant3.match(  //
        is_a(2.f).then([&]() {
            cat::exit(1);
        }),
        is_a(1.f).then([&]() {
            matched = true;
        }));
    verify(matched);
};
