#include <cat/linux>
#ifndef CAT_NO_VDSO
#include <cat/detail/vdso.hpp>
#endif

auto
nix::sys_getrandom(cat::span<unsigned char> buffer, getrandom_flags flags)
   -> nix::scaredy_nix<cat::idx> {
#ifndef CAT_NO_VDSO
   cat::maybe<cat::int8> const result = detail::vdso_getrandom(buffer, flags);
   if (result.has_value()) {
      if (result.value() >= 0) {
         return cat::idx(result.value());
      }
      if (result.value() != cat::int8(cat::to_underlying(linux_error::nosys))) {
         return static_cast<linux_error>(result.value());
      }
   }
   // If that function failed, fall back to the syscall.
#endif

   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      318, buffer.data(), buffer.size(), flags
   );
}
