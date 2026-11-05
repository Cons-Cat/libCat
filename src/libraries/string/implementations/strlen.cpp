#include <cat/string>

extern "C" {

[[deprecated("strlen() is deprecated! Use cat::string_length() instead.")]]
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
// ASan and TSan use their own `strlen()` by DLL injection, so we must prefer
// those.
[[gnu::visibility("hidden")]]
#endif
auto
strlen(char const* _Nonnull p_string) -> __SIZE_TYPE__ {
   return static_cast<__SIZE_TYPE__>(cat::string_length(p_string));
}

}  // extern "C"
