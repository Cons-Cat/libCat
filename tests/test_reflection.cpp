#include <type_list>
#include <variant>

void meow() {
    using Types = meta::TypeList<int, char>;
    using Type_0 = typename Types::Get<0>;
    using Type_1 = typename Types::Get<1>;

    static_assert(meta::is_same<Type_0, int>);
    static_assert(meta::is_same<Type_1, char>);

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

    cat::Variant<int, char, uint4> variant(int{1});
    Result(variant.holds_alternative<int>()).or_panic();
    int foo_int = variant.value<int>();
    Result(foo_int == 1).or_panic();

    static_assert(variant.type_index<int>() == 0u);
    static_assert(variant.type_index<char>() == 1u);
    static_assert(variant.type_index<uint4>() == 2u);

    variant = 'o';
    Result(variant.holds_alternative<char>()).or_panic();
    char foo_char = variant.value<char>();
    Result(foo_char == 'o').or_panic();

    cat::exit();
}
