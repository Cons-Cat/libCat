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

    Array<Vector, 4> vector_1;
    Array<Vector, 4> vector_2;
    Array<Vector, 4> additions;
    Array<int4, 4> masks;
    ssize length_iterator = string_1.size();
    ssize vector_size = ssizeof<Vector>();
    char const* p_string_1_iterator = string_1.p_data();
    char const* p_string_2_iterator = string_2.p_data();

    auto loop = [&](int size) -> bool {
        while (length_iterator >= vector_size * size) {
            for (int i = 0; i < size; ++i) {
                vector_1[i].load(string_1.p_data() + (i * size));
                vector_2[i].load(string_2.p_data() + (i * size));
                additions[i] = vector_1[i] + vector_2[i];
                masks[i] = move_mask(additions[i]);
            }

            for (int i = 0; i < size; ++i) {
                if (masks[i] == 0) {
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
