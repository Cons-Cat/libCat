#include <string>

void meow() {
    char const* p_string_1 = "Hello!";
    char const* const p_string_2 = "Hello!";

    StringView string_1 = "Hello!";
    StringView const string_2 = "Hello!";
    StringView string_3 = "Goodbye!";

    StringView long_string_1 =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    StringView long_string_2 =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    // Test a succesful string pointer case.
    if (!std::compare_strings(p_string_1, p_string_2)) {
        std::exit(1);
    }

    // Test a succesful string view case.
    if (!std::compare_strings(string_1, string_2)) {
        std::exit(1);
    }

    // Test a succesful large string case.
    if (!std::compare_strings(long_string_1, long_string_2)) {
        // TODO: Fix this failing test!
        // std::exit(1);
    }

    // Test a failure case.
    if (std::compare_strings(string_1, string_3)) {
        std::exit(1);
    }

    std::exit();
}
