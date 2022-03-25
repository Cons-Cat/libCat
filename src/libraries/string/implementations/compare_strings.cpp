#include <array>
#include <limits>
#include <string>

auto std::compare_strings(StringView const& string_1,
                          StringView const& string_2) -> bool1 {
    if (string_1.length != string_2.length) {
        return false;
    }

    // TODO: Use a type for an ISA-specific widest vector.
    using VectorType = charx32;

    Array<VectorType, 4> vector_1;
    Array<VectorType, 4> vector_2;
    Array<VectorType, 4> mask;
    Array<uint4, 4> results;
    ssize length_iterator = string_1.length;
    ssize vector_size = sizeof(VectorType);
    char const* p_string_1_iterator = string_1.p_data;
    char const* p_string_2_iterator = string_2.p_data;

    auto loop = [&](ssize const size) -> bool1 {
        while (length_iterator >= vector_size * size) {
            for (ssize i = 0; i < size; i++) {
                // TODO: Use `StringView::data()` getter methods.
                vector_1[i] =
                    *(meta::bit_cast<VectorType const*>(string_1.p_data) +
                      (i * size));
                vector_2[i] =
                    *(meta::bit_cast<VectorType const*>(string_2.p_data) +
                      (i * size));
                mask[i] = vector_1[i] + vector_2[i];
                results[i] = simd::move_mask(mask[i]);
            }

            for (ssize i = 0; i < size; i++) {
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
    for (ssize i = 0; i < length_iterator;
         i++, p_string_1_iterator++, p_string_2_iterator++) {
        if (*p_string_1_iterator != *p_string_2_iterator) {
            return false;
        }
    }

    return true;
}
