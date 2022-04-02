#include <linux>
#include <result>
#include <string>

auto cat::print(String const& string) -> Result<> {
    return nix::write(1, string.p_data, string.length);
}
