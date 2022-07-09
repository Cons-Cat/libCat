#include <cat/linux>
#include <cat/string>

auto cat::print(String const string) -> cat::OptionalNonNegative<ssize> {
    cat::Scaredy result =
        nix::sys_write(nix::FileDescriptor{1}, string.p_data(), string.size());
    if (result.has_value()) {
        return result.value();
    }
    return nullopt;
}
