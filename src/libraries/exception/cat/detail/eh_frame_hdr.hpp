#pragma once
#include <cat/arithmetic>
#include <cat/span>

namespace cat::detail {

struct fde_lookup_result {
   byte const* _Nullable p_fde = nullptr;
   uword instruction_pointer = 0u;
};

// Return the preceding index entry. The Call Frame Information parser checks
// its address range.
[[nodiscard]]
auto
find_fde_for_ip(uword instruction_pointer) noexcept(
   !__has_feature(cxx_exceptions)
) -> fde_lookup_result;

[[nodiscard]]
auto
eh_frame_span() noexcept(!__has_feature(cxx_exceptions)) -> span<byte const>;

[[nodiscard]]
auto
eh_except_table_span() noexcept(!__has_feature(cxx_exceptions))
   -> span<byte const>;

}
