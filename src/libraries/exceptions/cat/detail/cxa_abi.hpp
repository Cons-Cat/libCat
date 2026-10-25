#pragma once
#include <cat/detail/unwind.hpp>

namespace cat::detail {

// Metadata corresponding to each thrown exception.
struct alignas(__STDCPP_DEFAULT_NEW_ALIGNMENT__) exception_header {
   idx allocation_bytes;
   void const* type;
   void (*destructor)(void*);
   exception_header* next;
   int4 handlers;
   void* adjusted;
   unwind_exception unwind;
};

inline auto
header_from_unwind(unwind_exception* p_exception) -> exception_header* {
   return __builtin_bit_cast(
      exception_header*, (intptr<char>(__builtin_bit_cast(char*, p_exception))
                          - __builtin_offsetof(exception_header, unwind))
                            .get()
   );
}

inline auto
header_from_user(void* p_object) -> exception_header* {
   return static_cast<exception_header*>(p_object) - 1;
}

auto
exception_matches(void const* p_caught, void*& p_object, void const* p_thrown)
   -> bool;
}  // namespace cat::detail
