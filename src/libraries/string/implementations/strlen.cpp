#include <cat/string>
#include <stdint.h>

[[deprecated("strlen() is deprecated! Use cat::string_length() instead.")]] auto
strlen(char const* p_string) -> size_t {
    return static_cast<size_t>(cat::string_length(p_string));
}
