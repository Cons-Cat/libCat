#include <cat/array>
#include <cat/file>
#include <cat/linux>

auto
cat::get_executable_path(dyn_allocator allocator) -> maybe<file_path> {
   // TODO: This is not a robust way to get the executable path.
   // Under Steam or similar sandboxes, the path might completely differ,
   // and some environments like AppArmor or SELinux could disable access to
   // procfs path like `/proc/self/exe` entirely.
   //
   // On Linux, we can alternatively read `argv[0]` to get this executable path.

   // 1 KiB keeps this off the stack of a small clone thread. Paths are
   // conventionally bounded by a 4 KiB `PATH_MAX`, but that is a suggestion
   // rather than a kernel guarantee, so no buffer size is truncation-proof.
   array<char, 1'024u> buffer;
   nix::scaredy_nix<idx> const length =
      nix::sys_readlink("/proc/self/exe", span(buffer));

   // A path longer than the buffer truncates instead of failing, and partial
   // results are not worth returning. Failure means `/proc` is missing or
   // disabled, or, uncommonly, that a restrictive security module denies
   // access to `/proc/pid/exe`. Neither is transient.
   if (length.is_empty() || length.value() == buffer.size()) {
      return nullopt;
   }
   return make_file_path(allocator, str_view(buffer.data(), length.value()));
}
