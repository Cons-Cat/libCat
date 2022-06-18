#include <cat/linux>
#include <cat/string>
#include <result>

auto cat::print(cat::String const& string) -> cat::Optional<ssize> {
    cat::Scaredy result =
        nix::write(nix::FileDescriptor{1}, string.p_data(), string.size());
    if (result.has_value()) {
        return result.value();
    }
    return nullopt;
}
