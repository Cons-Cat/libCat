#include <cat/linux>
#include <cat/string>

auto cat::eprintln(string const string) -> ssize {
    // There is no reasonable way for a `write` syscall for `nix::stderr` to
    // fail, except by running out of buffer space, which fails gracefully
    // anyways.
    ssize output_length = nix::sys_write(nix::stderr, string).value();
    output_length += nix::sys_write(nix::stderr, "\n").value();
    return output_length;
}

auto cat::eprintln() -> ssize {
    return nix::sys_write(nix::stderr, "\n").value();
}
