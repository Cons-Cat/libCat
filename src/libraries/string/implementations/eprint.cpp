#include <cat/linux>
#include <cat/string>

auto cat::eprint(string const string) -> iword {
    // There is no reasonable way for a `write` syscall for `nix::stderr` to
    // fail, except by running out of buffer space, which fails gracefully
    // anyways.
    iword const output_length = nix::sys_write(nix::stderr, string).value();
    return output_length;
}
