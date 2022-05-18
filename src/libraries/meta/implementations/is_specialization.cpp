#include <cat/meta>

// https://stackoverflow.com/a/31763111
template <typename T, template <typename...> typename Template>
struct meta::is_specialization : meta::false_type {
    constexpr operator bool() {
        return false;
    };
};

template <template <typename...> typename Template, typename... Args>
struct meta::is_specialization<Template<Args...>, Template> : meta::true_type {
    constexpr operator bool() {
        return true;
    };
};
