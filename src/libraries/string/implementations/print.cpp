#include <cat/linux>
#include <cat/string>

auto cat::print(string const string) -> ssize {
    // There is no reasonable way for a `write` syscall for `nix::stdout` to
    // fail, except by running out of buffer space, which fails gracefully
    // anyways.
    return nix::sys_write(nix::stdout, string).value();
}
