#include <linux>
#include <result>
#include <string>

// TODO: Improve this performance with format strings when they are implemented.

auto std::print_line(char const* p_string) -> Result<> {
    auto result_1 = nix::write(1, p_string, std::string_length(p_string));
    if (!result_1.is_okay) {
        return result_1.failure_code;
    }
    return nix::write(1, "\n", 1);
}

auto std::print_line(StringView const& string) -> Result<> {
    auto result_1 = nix::write(1, string.p_data, string.length);
    if (!result_1.is_okay) {
        return result_1.failure_code;
    }
    return nix::write(1, "\n", 1);
}
