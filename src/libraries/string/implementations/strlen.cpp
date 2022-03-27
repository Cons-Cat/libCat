// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <catint.h>
#include <string>

[[deprecated(
    "strlen() is deprecated! Use cat::string_length<T>() instead.")]] auto
strlen(char const* p_string) -> size_t {
    return cat::string_length<size_t>(p_string);
}
