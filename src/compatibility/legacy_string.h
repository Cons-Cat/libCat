// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Legacy numerals are required for backwards-compatibility here.
#include <stdint.h>

[[deprecated("strlen() is deprecated! Use string_length() instead.")]] auto
strlen(char const* p_string) -> size_t {
    size_t result = 0;
    while (p_string[result] != '\0') {
        result++;
    }
    return result;
}
