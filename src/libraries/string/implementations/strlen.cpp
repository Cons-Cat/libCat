#include <cat/string>

[[deprecated("strlen() is deprecated! Use cat::string_length() instead.")]]
auto strlen(char const* p_string) -> __SIZE_TYPE__ {
    return static_cast<__SIZE_TYPE__>(cat::string_length(p_string));
}
