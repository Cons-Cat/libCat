// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <stdint.h>
#include <string.h>

[[deprecated(
    "strlen() is deprecated! Use simd::string_length<T>() instead.")]] auto
strlen(char8_t const* p_string) -> size_t {
    return simd::string_length_as<size_t>(p_string);
}
