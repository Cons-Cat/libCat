#pragma once

#include <cat/maybe>
#include <cat/span>
#include <cat/string>

namespace cat::detail {

struct source_location {
   str_view file;
   uint8 line = 0u;
};

[[nodiscard]]
auto
resolve_dwarf_line(span<byte const> image, uint8 address) -> source_location;

[[nodiscard]]
auto
decode_dwarf_form_size(
   span<byte const> bytes, uint8 form, idx offset_size = 4u,
   idx address_size = 8u
) -> maybe<idx>;

}  // namespace cat::detail
