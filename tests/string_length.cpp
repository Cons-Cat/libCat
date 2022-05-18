#include <cat/string>

void meow() {
    char const* p_string_1 = "Hello!";
    char const* const p_string_2 = "Hello!";

    char const* const p_string_3 = "Hello!";
    char const* p_string_4 = "Hello!";

    char const* const p_string_5 = "Hello!";
    char const* const p_string_6 = "Hello!";

    char const* p_string_7 = "/tmp/temp.sock";

    ssize len_1 = cat::string_length(p_string_1);
    ssize len_2 = cat::string_length(p_string_2);
    ssize len_3 = cat::string_length(p_string_3);
    ssize len_4 = cat::string_length(p_string_4);
    ssize len_5 = cat::string_length(p_string_5);
    ssize len_6 = cat::string_length(p_string_6);
    ssize len_7 = cat::string_length(p_string_7);

    Result(len_1 == len_2).or_panic();
    Result(len_1 == 7).or_panic();
    Result(len_3 == len_4).or_panic();
    Result(len_5 == len_6).or_panic();
    Result(len_7 == 15).or_panic();

    // Test `String`s.
    cat::String string_1 = p_string_1;
    Result(string_1.size() == len_1).or_panic();
    Result(string_1.slice(1, 4).size() == 3).or_panic();
    Result(string_1.first(4).size() == 4).or_panic();
    Result(string_1.last(3).size() == 3).or_panic();
    Result(cat::String("Hello!").size() == len_1).or_panic();

    cat::exit();
}
