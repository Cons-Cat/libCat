#include <cat/array>
#include <cat/simd>
#include <cat/string>

auto cat::compare_strings(String const string_1, String const string_2)
    -> bool {
    if (string_1.size() != string_2.size()) {
        return false;
    }

    // TODO: Use a type for an ISA-specific widest vector.
    using Vector = char1x32;

    Array<Vector, 4> vectors_1;
    Array<Vector, 4> vectors_2;
    Array<Vector, 4> subtractions;
    Array<int4, 4> masks;
    ssize length_iterator = string_1.size();
    ssize vector_size = ssizeof<Vector>();
    char const* p_string_1_iterator = string_1.data();
    char const* p_string_2_iterator = string_2.data();

    auto loop = [&](ssize size) -> bool {
        while (length_iterator >= vector_size * size) {
            for (int8::Raw i = 0; i < size; ++i) {
                vectors_1[i].load(string_1.data() + (i * size));
                vectors_2[i].load(string_2.data() + (i * size));
                // Subtract the characters from each other. If they are the
                // same, then every lane is 0.
                subtractions[i] = vectors_1[i] - vectors_2[i];
                masks[i] = move_mask(subtractions[i]);
            }

            for (ssize::Raw i = 0; i < size; ++i) {
                // If not every lane is 0.
                if (masks[i] != 0) {
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
    for (ssize i = 0; i < length_iterator;
         ++i, ++p_string_1_iterator, ++p_string_2_iterator) {
        if (*p_string_1_iterator != *p_string_2_iterator) {
            return false;
        }
    }

    return true;
}
