#include <cat/detail/vdso.hpp>

namespace {

struct vgetrandom_opaque_params {
   cat::uint4 state_size;
   cat::uint4 mmap_protections;
   cat::uint4 mmap_flags;
   cat::uint4 reserved[13];
};

static_assert(sizeof(vgetrandom_opaque_params) == 64);

using getrandom_hook = cat::int8 (*)(
   void* _Nullable, cat::uword, cat::uint4, void* _Nullable, cat::uword
);

getrandom_hook p_getrandom = nullptr;
void* p_getrandom_state = nullptr;
cat::uint4 getrandom_state_size = 0u;
bool getrandom_resolved = false;
bool getrandom_state_initialized = false;
bool getrandom_in_use = false;

void
initialize_getrandom_state() {
   getrandom_state_initialized = true;
   getrandom_hook const p_hook =
      __atomic_load_n(&p_getrandom, __ATOMIC_RELAXED);
   vgetrandom_opaque_params params{};
   cat::maybe<cat::int8> const query_result = nix::detail::call_vdso_hook(
      p_hook, nullptr, 0u, 0u, &params, cat::uword::max()
   );
   if (
      !query_result.has_value() || query_result.value() != 0
      || params.state_size == 0u || params.state_size > cat::page_size
   ) {
      return;
   }

   nix::scaredy_nix<cat::byte*> const mapping = nix::sys_mmap(
      nullptr, cat::page_size,
      static_cast<nix::memory_protection_flags>(params.mmap_protections.raw),
      static_cast<nix::memory_flags>(params.mmap_flags.raw),
      nix::invalid_file_descriptor, 0u
   );
   if (mapping.has_value()) {
      p_getrandom_state = mapping.value();
      getrandom_state_size = params.state_size;
   }
}

}  // namespace

auto
nix::detail::vdso_getrandom(
   cat::span<unsigned char> buffer, getrandom_flags flags
) -> cat::maybe<cat::int8> {
   getrandom_hook const p_hook = resolve_vdso_hook(
      p_getrandom, getrandom_resolved, "__vdso_getrandom", "getrandom"
   );
   if (
      p_hook == nullptr
      || __atomic_test_and_set(&getrandom_in_use, __ATOMIC_ACQUIRE)
   ) {
      return cat::nullopt;
   }

   if (!getrandom_state_initialized) {
      initialize_getrandom_state();
   }
   if (p_getrandom_state == nullptr) {
      __atomic_clear(&getrandom_in_use, __ATOMIC_RELEASE);
      return cat::nullopt;
   }

   cat::maybe<cat::int8> const result = call_vdso_hook(
      p_hook, buffer.data(), buffer.size(),
      cat::uint4(static_cast<unsigned int>(flags)), p_getrandom_state,
      getrandom_state_size
   );
   __atomic_clear(&getrandom_in_use, __ATOMIC_RELEASE);
   return result;
}
