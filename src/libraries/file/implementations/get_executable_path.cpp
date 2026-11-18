#include <cat/array>
#include <cat/file>
#include <cat/linux>

using cat::operator""_uki;

auto
cat::get_executable_path(allocator_dyn allocator) -> maybe<file_path> {
   // The customary (but not specified) `PATH_MAX` of Linux userspace runtimes
   // is conventionally 4 KiB. This is also the limit for file paths provided to
   // some syscalls, so it's a good upper bound to assume. Technically many
   // syscalls accept essentially unbounded length paths, though. We could
   // imagine a more robust implementation to Linux edge cases. Maybe a future
   // problem?

   // TODO: Use a `file_path_inplace` here.
   array<char, 4_uki> buffer;
   nix::scaredy_nix<idx> const length =
      nix::sys_readlink("/proc/self/exe", buffer);

   // If that fails, probably `/proc` is missing or disabled. These errors can't
   // be retried successfully as far as I know. If the buffer was merely too
   // small, then just propagate the same way.

   // TODO: Is that the right answer to the buffer being too small?
   if (length.is_empty() || length.value() == buffer.size()) {
      return nullopt;
   }
   return make_file_path(allocator, str_view(buffer.data(), length.value()));
}
