#include <linux>
#include <optional>
#include <result>
#include <string>

// TODO: Improve this performance with format strings when they are implemented.

auto cat::print_line(cat::String const& string) -> cat::Optional<ssize> {
    cat::Scaredy result_1 =
        nix::write(nix::FileDescriptor{1}, string.p_data(), string.size());
    if (!result_1.has_value()) {
        return nullopt;
    }
    cat::Scaredy result_2 = nix::write(nix::FileDescriptor{1}, "\n", 1);
    if (!result_2.has_value()) {
        return nullopt;
    }
    return result_1.value() + result_2.value();
}
