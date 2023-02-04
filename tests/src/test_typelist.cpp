#include <cat/type_list>

#include "../unit_tests.hpp"

TEST(test_typelist) {
    using types = cat::type_list<int, char>;
    using type_0 = types::get<0>;
    using type_1 = types::get<1>;

    static_assert(cat::is_same<type_0, int>);
    static_assert(cat::is_same<type_1, char>);

    static_assert(types::has_type<int>);
    static_assert(types::has_type<type_0>);
    static_assert(types::has_type<char>);
    static_assert(!types::has_type<bool>);

    static_assert(types::count_type<int> == 1);
    static_assert(!(types::count_type<int> == 2));
    static_assert(types::count_type<char> == 1);
    static_assert(types::count_type<bool> == 0);

    static_assert(types::is_unique<int>);
    static_assert(types::is_unique<char>);
    static_assert(!types::is_unique<bool>);

    using types2 = cat::type_list<int, int, char>;
    static_assert(types::is_unique_list);
    static_assert(!types2::is_unique_list);
    static_assert(types2::count_type<int> == 2);

    using concat_types = types::concat_types<types2>;
    static_assert(
        cat::is_same<concat_types, cat::type_list<int, char, int, int, char>>);

    using merge_types = types::merge<float, double>;
    static_assert(
        cat::is_same<merge_types, cat::type_list<int, char, float, double>>);

    using fill_types = cat::type_list_filled<int, 3>;
    static_assert(cat::is_same<fill_types, cat::type_list<int, int, int>>);
}
