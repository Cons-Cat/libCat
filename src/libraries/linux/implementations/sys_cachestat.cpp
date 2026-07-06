#include <cat/linux>

auto
nix::sys_cachestat(
   file_descriptor file_descriptor, cachestat_range const& range,
   cachestat_stats& out, cat::uint4 flags
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      451, file_descriptor, &range, &out, flags
   );
}
