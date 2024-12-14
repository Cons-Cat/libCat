#include <cat/linux>
#include <cat/string>

auto
cat::eprint(str_view const string) -> maybe_idx {
   // There is no reasonable way for a `write` syscall for `nix::stderr` to
   // fail, except by running out of buffer space, which fails gracefully
   // anyways.
   idx const output_length = nix::sys_write(nix::stderr, string).value();
   return output_length;
}
