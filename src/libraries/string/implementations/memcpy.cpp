#include <cat/string>

// `__SIZE_TYPE__` is a GCC macro.
extern "C" auto std::memcpy(void* p_destination, void const* p_source,
                            __SIZE_TYPE__ bytes) -> void* {
    cat::copy_memory(p_source, p_destination, static_cast<cat::iword>(bytes));
    return p_destination;
}
