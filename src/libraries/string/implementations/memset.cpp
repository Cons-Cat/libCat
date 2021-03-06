#include <cat/string>

// `__SIZE_TYPE__` is a GCC macro.
extern "C" auto std::memset(void* p_source, int byte_value, __SIZE_TYPE__ bytes)
    -> void* {
    cat::set_memory(p_source, static_cast<signed char>(byte_value),
                    static_cast<ssize>(bytes));
    return p_source;
}
