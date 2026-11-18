// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>

namespace nix::detail {

struct timeval {
   cat::int8 seconds;
   cat::int8 microseconds;
};

struct timezone {
   cat::int4 minutes_west;
   cat::int4 daylight_time;
};

void
initialize_vdso(void const* _Nullable p_elf_header);

// The kernel version from the vDSO's ELF note, if `initialize_vdso()` found it.
#ifndef CAT_BUILD_SHARED
[[gnu::visibility("hidden")]]
#endif
constinit inline cat::maybe<kernel_version> vdso_kernel_version;

[[nodiscard]]
auto
lookup_vdso_symbol(cat::str_view vdso_name, cat::str_view alias) noexcept
   -> void* _Nullable;

// `Hook` is a function type, like `int(int)`.
template <typename Hook>
[[nodiscard]]
auto
resolve_vdso_hook(
   Hook* _Nullable& p_hook, bool& resolved, cat::str_view vdso_name,
   cat::str_view alias
) noexcept -> Hook* _Nullable {
   if (!__atomic_load_n(&resolved, __ATOMIC_ACQUIRE)) {
      Hook* const result =
         __builtin_bit_cast(Hook*, lookup_vdso_symbol(vdso_name, alias));
      __atomic_store_n(&p_hook, result, __ATOMIC_RELAXED);
      __atomic_store_n(&resolved, true, __ATOMIC_RELEASE);
   }
   return __atomic_load_n(&p_hook, __ATOMIC_RELAXED);
}

template <typename Hook, typename... Args>
[[nodiscard]]
auto
call_vdso_hook(Hook* _Nullable p_hook, Args... arguments) noexcept
   -> cat::maybe<decltype(p_hook(arguments...))> {
   if (p_hook == nullptr) {
      return cat::nullopt;
   }
   return p_hook(arguments...);
}

[[nodiscard]]
auto
vdso_clock_gettime(clock_id clock, timespec& out) -> cat::maybe<cat::int4>;

[[nodiscard]]
auto
vdso_clock_getres(clock_id clock, timespec& out) -> cat::maybe<cat::int4>;

[[nodiscard]]
auto
vdso_gettimeofday(timeval* _Nullable p_time, timezone* _Nullable p_timezone)
   -> cat::maybe<cat::int4>;

[[nodiscard]]
auto
vdso_time(cat::int8* _Nullable p_time) -> cat::maybe<cat::int8>;

[[nodiscard]]
auto
vdso_getrandom(cat::span<unsigned char> buffer, getrandom_flags flags)
   -> cat::maybe<cat::int8>;

}  // namespace nix::detail
