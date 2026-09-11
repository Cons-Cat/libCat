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

[[nodiscard]]
auto
lookup_vdso_symbol(
   char const* _Nonnull p_vdso_name, char const* _Nonnull p_alias
) -> void* _Nullable;

template <typename Hook>
[[nodiscard]]
auto
resolve_vdso_hook(
   Hook& p_hook, bool& resolved, char const* _Nonnull p_vdso_name,
   char const* _Nonnull p_alias
) -> Hook {
   if (!__atomic_load_n(&resolved, __ATOMIC_ACQUIRE)) {
      Hook const result =
         __builtin_bit_cast(Hook, lookup_vdso_symbol(p_vdso_name, p_alias));
      __atomic_store_n(&p_hook, result, __ATOMIC_RELAXED);
      __atomic_store_n(&resolved, true, __ATOMIC_RELEASE);
   }
   return __atomic_load_n(&p_hook, __ATOMIC_RELAXED);
}

template <typename Hook, typename... Args>
[[nodiscard]]
auto
call_vdso_hook(Hook p_hook, Args... arguments)
   -> cat::maybe<decltype(p_hook(arguments...))> {
   if (p_hook == nullptr) {
      return cat::nullopt;
   }
   asm volatile("" ::
                   : "memory");
   auto const result = p_hook(arguments...);
   asm volatile("" ::
                   : "memory");
   return result;
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
