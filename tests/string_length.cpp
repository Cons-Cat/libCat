#include <string>

void meow() {
    char const* p_string_1 = "Hello!";
    char const* const p_string_2 = "Hello!";

    char const* const p_string_3 = "Hello!";
    char const* p_string_4 = "Hello!";

    char const* const p_string_5 = "Hello!";
    char const* const p_string_6 = "Hello!";

    char const* p_string_7 = "/tmp/temp.sock";

    ssize len_1 = std::string_length(p_string_1);
    ssize len_2 = std::string_length(p_string_2);
    ssize len_3 = std::string_length(p_string_3);
    ssize len_4 = std::string_length(p_string_4);
    ssize len_5 = std::string_length(p_string_5);
    ssize len_6 = std::string_length(p_string_6);
    ssize len_7 = std::string_length(p_string_7);

    Result(len_1 == len_2).or_panic();
    Result(len_3 == len_4).or_panic();
    Result(len_5 == len_6).or_panic();
    Result(len_7 == 14).or_panic();

    std::exit();
}
