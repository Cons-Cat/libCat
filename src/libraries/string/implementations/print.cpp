#include <linux>
#include <result>
#include <string>

auto std::print(char const* p_string) -> Result<> {
    return write(1, p_string, std::string_length(p_string));
}

auto std::print(StringView const string) -> Result<> {
    return write(1, string.p_data, string.length);
}
