// -*- mode: c++ -*-
// vim: set ft=cpp:

#include <cat/page_allocator>

// I have moved these into a standalone translation unit so that debuggers can
// call them.

namespace cat {

auto
page_allocator::bytes_used() -> idx {
   scaredy const smaps = nix::read_self_anon_smaps();
   return smaps.has_value() ? smaps.value().resident_bytes : 0u;
}

auto
page_allocator::bytes_capacity() -> idx {
   scaredy const smaps = nix::read_self_anon_smaps();
   return smaps.has_value() ? smaps.value().mapped_bytes : 0u;
}

}  // namespace cat
