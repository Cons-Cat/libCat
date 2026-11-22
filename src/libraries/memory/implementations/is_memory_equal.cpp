#include <cat/memory>

#include "scan_memory.tpp"

namespace cat::detail {

auto
is_memory_equal_impl(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx bytes
) noexcept(!__has_feature(cxx_exceptions)) -> bool {
   if (p_lhs == p_rhs) {
      return true;
   }

   return !scan_memory<memory_scan::equality>(p_lhs, p_rhs, bytes);
}

}  // namespace cat::detail
