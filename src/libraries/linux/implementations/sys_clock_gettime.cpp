#include <cat/linux>
#ifndef CAT_NO_VDSO
#include <cat/detail/vdso.hpp>
#endif

auto
nix::sys_clock_gettime(clock_id clock, timespec& out)
   -> nix::scaredy_nix<void> {
#ifndef CAT_NO_VDSO
   cat::maybe<cat::int4> const result = detail::vdso_clock_gettime(clock, out);
   if (result.has_value()) {
      // `scaredy_nix` reads a negative value as a `linux_error`, just as it
      // does for a syscall's return.
      return scaredy_nix<void>(result.value());
   }
   // If that function failed, fall back to the syscall.
#endif

   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(228, clock, &out);
}
