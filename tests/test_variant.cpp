#include <cat/variant>

auto main() -> int {
    cat::Variant<int, char, uint4> variant(int{1});
    Result(variant.is<int>()).or_exit();
    Result(variant.holds_alternative<int>()).or_exit();
    int foo_int = variant.get<int>();
    Result(foo_int == 1).or_exit();

    static_assert(variant.alternative_index<int> == 0u);
    static_assert(variant.alternative_index<char> == 1u);
    static_assert(variant.alternative_index<uint4> == 2u);

    variant = 'o';
    Result(variant.is<char>()).or_exit();
    Result(variant.holds_alternative<char>()).or_exit();
    char foo_char = variant.get<char>();
    Result(foo_char == 'o').or_exit();

    cat::Optional<char&> opt1 = variant.get_if<char>();
    Result(opt1.has_value()).or_exit();
    cat::Optional opt2 = variant.get_if<int>();
    Result(!opt2.has_value()).or_exit();

    // Test variant subtype constructor and assignment operator.
    cat::Variant<int, char, uint4, int2> variant2 = variant;
    Result(variant2.is<char>()).or_exit();
    Result(variant2.holds_alternative<char>()).or_exit();
    variant2 = 1;
    Result(variant2.is<int>()).or_exit();
    Result(variant2.holds_alternative<int>()).or_exit();
    variant2 = variant;
    Result(variant2.is<char>()).or_exit();
    Result(variant2.holds_alternative<char>()).or_exit();

    variant = 1;
    cat::Variant<int, char, uint4, int2> variant3 = variant;
    Result(variant3.is<int>()).or_exit();
    Result(variant3.holds_alternative<int>()).or_exit();
    variant3 = int2{10};
    Result(variant3.is<int2>()).or_exit();
    Result(variant3.holds_alternative<int2>()).or_exit();
    variant3 = variant;
    Result(variant3.is<int>()).or_exit();
    Result(variant3.holds_alternative<int>()).or_exit();

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
    Result(variant3.is<int>()).or_exit();
    Result(variant3.is(1)).or_exit();

    variant3 = 'b';
    Result(variant3.is<char>()).or_exit();
    Result(variant3.is('b')).or_exit();

    // `.is()` accepts unconditionally invalid types, unlike
    // `.holds_alternative()`.
    Result(!variant3.is<unsigned long long>()).or_exit();

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
    Result(matched).or_exit();

    matched = false;
    cat::match(variant3)(  //
        is_a<float>().then([&]() {
            cat::exit(1);
        }),
        is_a<int>().then([&]() {
            matched = true;
        }));
    Result(matched).or_exit();

    // `variant3` holds an integer, but floats are convertible to integers.
    matched = false;
    cat::match(variant3)(  //
        is_a(2.f).then([&]() {
            cat::exit(1);
        }),
        is_a(1.f).then([&]() {
            matched = true;
        }));
    Result(matched).or_exit();

    // Test member access pattern matching syntax.
    matched = false;
    variant3.match(  //
        is_a(2.f).then([&]() {
            cat::exit(1);
        }),
        is_a(1.f).then([&]() {
            matched = true;
        }));
    Result(matched).or_exit();
};
