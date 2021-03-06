#include <cat/linux>
#include <cat/string>

auto cat::println(String const string) -> ssize {
    // There is no reasonable way for a `write` syscall for `nix::stdout` to
    // fail, except by running out of buffer space.
    ssize output_length = nix::sys_write(nix::stdout, string).value();
    output_length += nix::sys_write(nix::stdout, "\n").value();
    return output_length;
}
