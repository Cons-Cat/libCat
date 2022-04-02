#include <linux>
#include <result>
#include <string>

// TODO: Improve this performance with format strings when they are implemented.

auto cat::print_line(String const& string) -> Result<> {
    auto result = nix::write(1, string.p_data, string.length);
    if (!result.is_okay) {
        return Failure(result.failure_code);
    }
    return nix::write(1, "\n", 1);
}
