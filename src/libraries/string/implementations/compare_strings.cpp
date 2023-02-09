#include <cat/array>
#include <cat/simd>
#include <cat/string>

auto cat::compare_strings(string const string_1, string const string_2)
    -> bool {
    if (string_1.size() != string_2.size()) {
        return false;
    }

    // TODO: Use a type for an ISA-specific widest vector.
    using vector_simd = char1x32;

    array<vector_simd, 4> vectors_1;
    array<vector_simd, 4> vectors_2;
    array<vector_simd::mask_type, 4> comparisons;
    iword length_iterator = string_1.size();
    iword vector_size = ssizeof(vector_simd);
    char const* p_string_1_iterator = string_1.data();
    char const* p_string_2_iterator = string_2.data();

    auto loop = [&](iword size) -> bool {
        while (length_iterator >= vector_size * size) {
            for (iword i = 0; i < size; ++i) {
                vectors_1[i].load(string_1.data() + (i * size));
                vectors_2[i].load(string_2.data() + (i * size));
                comparisons[i] = (vectors_1[i] == vectors_2[i]);
            }

            for (iword i = 0; i < size; ++i) {
                // If any lanes are not equal to each other:
                if (!comparisons[i].all_of()) {
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

    // TODO: Extract this to a scalar function.
    // Compare remaining characters individually.
    for (iword i = 0; i < length_iterator;
         ++i, ++p_string_1_iterator, ++p_string_2_iterator) {
        if (*p_string_1_iterator != *p_string_2_iterator) {
            return false;
        }
    }

    return true;
}
