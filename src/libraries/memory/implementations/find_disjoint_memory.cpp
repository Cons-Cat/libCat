#include <cat/memory>

#include "scan_memory.tpp"

namespace cat::detail {

auto
find_disjoint_memory_impl(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx bytes
) noexcept(!__has_feature(cxx_exceptions)) -> idx {
   if (p_lhs == p_rhs) {
      return bytes;
   }

   maybe<idx> const mismatch =
      scan_memory<memory_scan::mismatch>(p_lhs, p_rhs, bytes);
   return mismatch.has_value() ? mismatch.value() : bytes;
}

}  // namespace cat::detail
