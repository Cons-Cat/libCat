#include <cat/string>

extern "C" [[clang::no_builtin]]
auto
std::memset(void* p_source, int byte_value, __SIZE_TYPE__ bytes) -> void* {
    if (bytes > 8u) {
        cat::set_memory(p_source, static_cast<unsigned char>(byte_value),
                        bytes);
    } else {
        // TODO: Clang 19 inserts `memset()` calls as some structs' `default`
        // assignment operators. The vectorized `set_memory` causes some
        // problems with that, so this trivial loop is done insead. Find a
        // better solution.
        for (unsigned i = 0; i < bytes; ++i) {
            *((unsigned char*)p_source) = byte_value;
        }
    }
    return p_source;
}
