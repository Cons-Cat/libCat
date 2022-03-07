#include <string>

void meow() {
    char8_t const* p_string = u8"Hello!";
    int4 scalar_len = std::string_length<int4>(p_string);
    int4 vector_len = std::string_length<int4>(p_string);
    Result(scalar_len == vector_len).or_print_panic();
    exit(0);
}
