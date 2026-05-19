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
nix::get_kernel_version() -> kernel_version {
   utsname uts{};
   if (auto result = sys_uname(uts); !result.has_value()) {
      // Treat as 0.0 on the rare case the syscall itself fails. The
      // startup probe falls back to "no feature present" for every
      // version-gated check.
      return {.major = 0, .minor = 0};
   }

   char const* _Nonnull p_cursor = uts.release.data();
   kernel_version parsed{};
   parsed.major = parse_decimal(p_cursor);
   if (*p_cursor == '.') {
      ++p_cursor;
      parsed.minor = parse_decimal(p_cursor);
   }
   return parsed;
}
