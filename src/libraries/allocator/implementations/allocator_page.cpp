// -*- mode: c++ -*-
// vim: set ft=cpp:

#include <cat/allocator_page>

// I have moved these into a standalone translation unit so that debuggers can
// call them.

namespace cat {

auto
allocator_page::bytes_used() -> idx {
   scaredy const smaps = nix::read_self_anon_smaps();
   return smaps.has_value() ? smaps.value().resident_bytes : 0u;
}

auto
allocator_page::bytes_capacity() -> idx {
   scaredy const smaps = nix::read_self_anon_smaps();
   return smaps.has_value() ? smaps.value().mapped_bytes : 0u;
}

}  // namespace cat
