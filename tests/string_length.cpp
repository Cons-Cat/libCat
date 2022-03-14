#include <string>

void meow() {
    char const* p_string_1 = "Hello!";
    char const* const p_string_2 = "Hello!";

    char const* const p_string_3 = "Hello!";
    char const* p_string_4 = "Hello!";

    char const* const p_string_5 = "Hello!";
    char const* const p_string_6 = "Hello!";

    isize len_1 = std::string_length(p_string_1);
    isize len_2 = std::string_length(p_string_2);
    isize len_3 = std::string_length(p_string_3);
    isize len_4 = std::string_length(p_string_4);
    isize len_5 = std::string_length(p_string_5);
    isize len_6 = std::string_length(p_string_6);

    Result(len_1 == len_2).or_panic();
    Result(len_3 == len_4).or_panic();
    Result(len_5 == len_6).or_panic();

    std::exit();
}
