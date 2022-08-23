#include <cat/type_list>
#include <cat/variant>

auto main() -> int {
    using Types = cat::TypeList<int, char>;
    using Type_0 = typename Types::Get<0>;
    using Type_1 = typename Types::Get<1>;

    static_assert(cat::is_same<Type_0, int>);
    static_assert(cat::is_same<Type_1, char>);

    static_assert(Types::has_type<int>);
    static_assert(Types::has_type<Type_0>);
    static_assert(Types::has_type<char>);
    static_assert(!Types::has_type<bool>);

    static_assert(Types::count_type<int> == 1);
    static_assert(!(Types::count_type<int> == 2));
    static_assert(Types::count_type<char> == 1);
    static_assert(Types::count_type<bool> == 0);

    static_assert(Types::is_unique<int>);
    static_assert(Types::is_unique<char>);
    static_assert(!Types::is_unique<bool>);

    using Types2 = cat::TypeList<int, int, char>;
    static_assert(Types::is_unique_list);
    static_assert(!Types2::is_unique_list);
    static_assert(Types2::count_type<int> == 2);

    using ConcatTypes = Types::Concat<Types2>;
    static_assert(
        cat::is_same<ConcatTypes, cat::TypeList<int, char, int, int, char>>);

    using MergeTypes = Types::Merge<float, double>;
    static_assert(
        cat::is_same<MergeTypes, cat::TypeList<int, char, float, double>>);

    using FillTypes = cat::TypeListFilled<int, 3>;
    static_assert(cat::is_same<FillTypes, cat::TypeList<int, int, int>>);

    cat::Variant<int, char, uint4> variant(int{1});
    Result(variant.holds_alternative<int>()).or_exit();
    int foo_int = variant.get<int>();
    Result(foo_int == 1).or_exit();

    static_assert(variant.index<int> == 0u);
    static_assert(variant.index<char> == 1u);
    static_assert(variant.index<uint4> == 2u);

    variant = 'o';
    Result(variant.holds_alternative<char>()).or_exit();
    char foo_char = variant.get<char>();
    Result(foo_char == 'o').or_exit();

    cat::Optional opt1 = variant.get_if<char>();
    Result(opt1.has_value()).or_exit();
    cat::Optional opt2 = variant.get_if<int>();
    Result(!opt2.has_value()).or_exit();

    // Test variant subtype constructor and assignment operator.
    cat::Variant<int, char, uint4, int2> variant2 = variant;
    Result(variant2.holds_alternative<char>()).or_exit();
    variant2 = 1;
    Result(variant2.holds_alternative<int>()).or_exit();
    variant2 = variant;
    Result(variant2.holds_alternative<char>()).or_exit();

    variant = 1;
    cat::Variant<int, char, uint4, int2> variant3 = variant;
    Result(variant3.holds_alternative<int>()).or_exit();
    variant3 = int2{10};
    Result(variant3.holds_alternative<int2>()).or_exit();
    variant3 = variant;
    Result(variant3.holds_alternative<int>()).or_exit();

    // Test getting variant type by index.
    static_assert(cat::is_same<decltype(variant3.get<0>()), int>);
    static_assert(cat::is_same<decltype(variant3.get<1>()), char>);
    static_assert(cat::is_same<decltype(variant3.get<2>()), uint4>);
    static_assert(cat::is_same<decltype(variant3.get<3>()), int2>);
}
