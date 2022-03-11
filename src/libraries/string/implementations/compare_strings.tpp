// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <buffer>
#include <limits>
#include <string>

auto std::compare_strings(StringView const& string_1,
                          StringView const& string_2) -> bool {
    if (string_1.length != string_2.length) {
        return false;
    }

    // TODO: Use a type for an ISA-specific widest vector.
    using VectorType = charx32;

    Buffer<VectorType, 4> vector_1;
    Buffer<VectorType, 4> vector_2;
    Buffer<VectorType, 4> mask;
    Buffer<uint4, 4> results;
    isize length_iterator = string_1.length;
    isize vector_size = sizeof(VectorType);
    char const* p_string_1_iterator = string_1.p_data;
    char const* p_string_2_iterator = string_2.p_data;

    auto loop = [&](isize size) -> bool {
        while (length_iterator >= vector_size * size) {
            for (isize i = 0; i < size; i++) {
                vector_1[i] = *(meta::bit_cast<VectorType*>(string_1.p_data) +
                                (i * size));
                vector_2[i] = *(meta::bit_cast<VectorType*>(string_2.p_data) +
                                (i * size));
                mask[i] = vector_1[i] + vector_2[i];
                results[i] = simd::move_mask(mask[i]);
            }

            for (isize i = 0; i < size; i++) {
                if (results[i] != std::numeric_limits<uint4>::max()) {
                    return false;
                }
            }

            length_iterator -= vector_size * size;
            p_string_1_iterator += vector_size * size;
            p_string_2_iterator += vector_size * size;
        }

        return true;
    };

    // Compare four, two, then one vectors of characters at a time.
    if (!loop(4)) {
        return false;
    }
    if (!loop(2)) {
        return false;
    }
    if (!loop(1)) {
        return false;
    }

    // Compare remaining characters individually.
    for (isize i = 0; i < length_iterator;
         i++, p_string_1_iterator++, p_string_2_iterator++) {
        if (*p_string_1_iterator != *p_string_2_iterator) {
            return false;
        }
    }

    return true;
}
