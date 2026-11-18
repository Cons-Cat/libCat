#ifndef CAT_NO_VDSO
#include <cat/detail/vdso.hpp>
#endif
#include <cat/linux>

namespace {
auto
parse_decimal(char const* _Nonnull& p_cursor) -> cat::int4 {
   cat::int4 result = 0;
   while (*p_cursor >= '0' && *p_cursor <= '9') {
      result = result * 10 + (*p_cursor - '0');
      ++p_cursor;
   }
   return result;
}
}  // namespace

auto
nix::get_kernel_version() noexcept -> kernel_version {
#ifndef CAT_NO_VDSO
   if (detail::vdso_kernel_version.has_value()) {
      return detail::vdso_kernel_version.value();
   }
#endif
   return detail::get_uname_kernel_version();
}

auto
nix::detail::get_uname_kernel_version() noexcept -> kernel_version {
   utsname uts{};
   if (auto result = sys_uname(uts); result.is_empty()) {
      // Evaluate to 0.0 if somehow this fails.
      return {0, 0};
   }

   char const* _Nonnull p_cursor = uts.release.data();
   kernel_version parsed{};
   auto& [major, minor] = parsed;
   major = parse_decimal(p_cursor);
   if (*p_cursor == '.') {
      ++p_cursor;
      minor = parse_decimal(p_cursor);
   }
   return parsed;
}
