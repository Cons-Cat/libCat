#include <cat/linux>
#include <cat/string>

auto
cat::eprintln(str_view const string) -> maybe_idx {
   // There is no reasonable way for a `write` syscall for `nix::stderr` to
   // fail, except by running out of buffer space, which fails gracefully
   // anyways.
   idx output_length = nix::sys_write(nix::stderr, string).value();
   output_length += nix::sys_write(nix::stderr, "\n").value();
   return output_length;
}

auto
cat::eprintln() -> maybe_idx {
   return nix::sys_write(nix::stderr, "\n").value();
}
