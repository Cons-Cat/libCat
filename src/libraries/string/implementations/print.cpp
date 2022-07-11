#include <cat/linux>
#include <cat/string>

auto cat::print(String const string) -> ssize {
    // There is no reasonable way for a `write` syscall for `nix::stdout` to
    // fail, except by running out of buffer space.
    ssize const output_length = nix::sys_write(nix::stdout, string).value();
    return output_length;
}
