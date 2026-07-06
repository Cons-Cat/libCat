// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/meta>

namespace x64 {

// Forward `rep movsb` byte copy.
template <
   cat::is_implicit_lifetime Destination, cat::is_implicit_lifetime Source>
void
movsb(
   Destination* _Nonnull p_destination, Source const* _Nonnull p_source,
   cat::idx bytes
) {
   asm volatile(R"(rep movsb)"
                : "+D"(p_destination), "+S"(p_source), "+c"(bytes)
                :
                : "memory");
}

// Backward `rep movsb` copy. `p_destination` must be after `p_source`.
template <
   cat::is_implicit_lifetime Destination, cat::is_implicit_lifetime Source>
void
movsb_backward(
   Destination* _Nonnull p_destination, Source const* _Nonnull p_source,
   cat::idx bytes
) {
   unsigned char* p_dest = reinterpret_cast<unsigned char*>(p_destination);
   unsigned char const* p_src =
      reinterpret_cast<unsigned char const*>(p_source);

   p_dest += bytes - 1u;
   p_src += bytes - 1u;

   asm volatile(R"(
        std
        rep movsb
        cld)"
                : "+D"(p_dest), "+S"(p_src), "+c"(bytes)
                :
                : "memory");
}

// `rep stosb` byte fill.
template <cat::is_implicit_lifetime Destination>
void
stosb(
   Destination* _Nonnull p_destination, cat::byte const byte_value,
   cat::idx bytes
) {
   asm volatile(R"(rep stosb)"
                : "+D"(p_destination), "+c"(bytes)
                : "a"(byte_value)
                : "memory");
}

}  // namespace x64
