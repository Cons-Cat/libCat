#include <cat/string>

// `__SIZE_TYPE__` is a GCC macro.
// [[deprecated("memcpy() is deprecated! Use cat::copy_memory() instead!")]]
extern "C" auto std::memcpy(void* p_destination, void const* p_source,
                            __SIZE_TYPE__ bytes) -> void* {
    cat::copy_memory(p_source, p_destination, static_cast<ssize>(bytes));
    return p_destination;
}
