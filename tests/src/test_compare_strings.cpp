#include <cat/string>

#include "../unit_tests.hpp"

TEST(test_compare_strings) {
    char const* p_string_1 = "Hello!";
    char const* const p_string_2 = "Hello!";

    cat::str_span string_1 = "Hello!";
    cat::str_span const string_2 = "Hello!";
    cat::str_span string_3 = "Goodbye!";

    cat::str_span long_string_1 =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    cat::str_span long_string_2 =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    // Test a succesful string pointer case.
    cat::verify(cat::compare_strings(p_string_1, p_string_2));

    // Test a succesful string case.
    cat::verify(cat::compare_strings(string_1, string_2));

    // Test a succesful large string case.
    cat::verify(cat::compare_strings(long_string_1, long_string_2));

    // Test a failure case.
    cat::verify(!cat::compare_strings(string_1, string_3));

    [[maybe_unused]]
    cat::str_span const_string_1 = "Hello, ";
    [[maybe_unused]]
    constexpr cat::str_span const_string_2 = "world!";

    // Fixed length strings.
    constexpr cat::str_inplace const_string_3 = "Hello, ";
    constexpr cat::str_inplace const_string_4 = "world!";

    // Test collection operations.
    auto _ = const_string_1[1];
    cat::verify(!const_string_3.at(10).has_value());

    // TODO: Make this `constexpr`.
    cat::str_inplace hello_world = (const_string_3 + const_string_4);
    constexpr cat::str_inplace const_hello_world =
        (const_string_3 + const_string_4);
    cat::verify(cat::compare_strings(hello_world, "Hello, world!"));
    cat::verify(cat::compare_strings(const_hello_world, "Hello, world!"));

    iword const h = const_string_1.find('H').value();
    iword const e = const_string_1.find('e').value();
    iword const l = const_string_1.find('l').value();
    iword const o = const_string_1.find('o').value();
    cat::verify(h == 0);
    cat::verify(e == 1);
    cat::verify(l == 2);
    cat::verify(o == 4);
}
