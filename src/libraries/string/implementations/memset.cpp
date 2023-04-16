#include <cat/string>

// `__SIZE_TYPE__` is a GCC macro.
extern "C" auto std::memset(void* p_source, int byte_value, __SIZE_TYPE__ bytes)
    -> void* {
    cat::set_memory(p_source, static_cast<unsigned char>(byte_value),
                    static_cast<cat::uword>(bytes));
    return p_source;
}
