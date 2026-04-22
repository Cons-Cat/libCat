#include <cat/string>

extern "C" {

[[deprecated("strlen() is deprecated! Use cat::string_length() instead.")]]
#if __has_feature(address_sanitizer)
// asan has its own `strlen` shim that interposes the libc one. We should prefer
// that.
[[gnu::visibility("hidden")]]
#endif
auto
strlen(char const* p_string) -> __SIZE_TYPE__ {
   return static_cast<__SIZE_TYPE__>(cat::string_length(p_string));
}

}  // extern "C"
