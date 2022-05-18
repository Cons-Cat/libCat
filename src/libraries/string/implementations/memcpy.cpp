#include <cat/memory>

[[deprecated("memcpy() is deprecated! Use simd::copy_buffer() instead!")]] auto
memcpy(void* p_destination, void const* p_source, usize bytes) -> void* {
    cat::copy_memory(p_source, p_destination, static_cast<ssize>(bytes));
    return p_destination;
}
