#include <linux>
#include <result>
#include <string>

auto cat::print(StringView const& string) -> Result<> {
    return nix::write(1, string.p_data, string.length);
}
