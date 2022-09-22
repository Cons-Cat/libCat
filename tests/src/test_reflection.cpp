#include <cat/type_list>

auto main() -> int {
    using Types = cat::TypeList<int, char>;
    using Type0 = typename Types::Get<0>;
    using Type1 = typename Types::Get<1>;

    static_assert(cat::is_same<Type0, int>);
    static_assert(cat::is_same<Type1, char>);

    static_assert(Types::has_type<int>);
    static_assert(Types::has_type<Type0>);
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
}
