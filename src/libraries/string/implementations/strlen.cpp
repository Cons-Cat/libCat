// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <stdint.h>
#include <string>

[[deprecated(
    "strlen() is deprecated! Use std::string_length<T>() instead.")]] auto
strlen(char8_t const* p_string) -> size_t {
    return std::string_length<size_t>(p_string);
}
