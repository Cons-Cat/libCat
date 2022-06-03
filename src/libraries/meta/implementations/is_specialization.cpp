#include <cat/meta>

// https://stackoverflow.com/a/31763111
template <typename T, template <typename...> typename Template>
struct cat::is_specialization : cat::false_type {
    constexpr operator bool() {
        return false;
    };
};

template <template <typename...> typename Template, typename... Args>
struct cat::is_specialization<Template<Args...>, Template> : cat::true_type {
    constexpr operator bool() {
        return true;
    };
};
