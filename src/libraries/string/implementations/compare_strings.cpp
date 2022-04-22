#include <array>
#include <limits>
#include <string>

auto cat::compare_strings(String const& string_1, String const& string_2)
    -> bool1 {
    if (string_1.size() != string_2.size()) {
        return false;
    }

    // TODO: Use a type for an ISA-specific widest vector.
    using VectorType = char1x32;

    Array<VectorType, 4> vector_1;
    Array<VectorType, 4> vector_2;
    Array<VectorType, 4> mask;
    Array<uint4, 4> results;
    ssize length_iterator = string_1.size();
    ssize vector_size = sizeof(VectorType);
    char const* p_string_1_iterator = string_1.p_data();
    char const* p_string_2_iterator = string_2.p_data();

    auto loop = [&](int const size) -> bool1 {
        while (length_iterator >= vector_size * size) {
            for (int i = 0; i < size; i++) {
                // TODO: Use `String::data()` getter methods.
                vector_1[i] =
                    *(static_cast<VectorType const*>(
                          static_cast<void const*>(string_1.p_data())) +
                      (i * size));
                vector_2[i] =
                    *(static_cast<VectorType const*>(
                          static_cast<void const*>(string_2.p_data())) +
                      (i * size));
                mask[i] = vector_1[i] + vector_2[i];
                results[i] = simd::move_mask(mask[i]);
            }

            for (int i = 0; i < size; i++) {
                if (results[i] != cat::numeric_limits<uint4>::max()) {
                    return false;
                }
            }

            length_iterator -= vector_size * size;
            p_string_1_iterator += vector_size.c() * size;
            p_string_2_iterator += vector_size.c() * size;
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
