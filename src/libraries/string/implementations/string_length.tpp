// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/simd>
#include <cat/string>
#include <cat/utility>

// This function requires SSE4.2, unless it is used in a `constexpr` context.
constexpr auto cat::string_length(char const* p_string) -> ssize {
    if consteval {
        ssize result = 0;
        while (true) {
            if (p_string[result.raw] == '\0') {
                return result;
            }
            result++;
        }
    } else {
        // TODO: Implement with portable SIMD, and tune performance.
        constexpr char1x16 zeros = '\0';

        for (ssize i = 0;; i += 16) {
            char1x16 const data = char1x16::loaded(p_string + i);
            constexpr x64::string_control mask =
                x64::string_control::unsigned_byte |
                x64::string_control::compare_equal_each |
                x64::string_control::least_significant;

            // If there are one or more `0` bytes in `data`:
            if (x64::compare_implicit_length_strings<mask>(data, zeros)) {
                int4 const index =
                    x64::compare_implicit_length_strings_return_index<mask>(
                        data, zeros);
                // Adding `1` is required to count the null terminator.
                return i + index + 1;
            }
        }

        __builtin_unreachable();
    }
}
