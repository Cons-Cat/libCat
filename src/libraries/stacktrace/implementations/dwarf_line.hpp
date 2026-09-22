#pragma once

#include <cat/allocator>
#include <cat/maybe>
#include <cat/span>
#include <cat/string>
#include <cat/vec>

namespace cat::detail {

struct source_location {
   str_view file;
   uint8 line = 0u;
};

struct dwarf_line_sequence {
   uint8 begin = 0u;
   uint8 end = 0u;
   idx unit = 0u;
   idx program = 0u;
};

struct dwarf_line_context {
   span<byte const> line;
   span<byte const> line_strings;
   span<byte const> strings;
   vec_big<dwarf_line_sequence> sequences;
   bool has_sequence_index = false;
};

[[nodiscard]]
auto
make_dwarf_line_context(span<byte const> elf, allocator_dyn allocator)
   -> dwarf_line_context;

[[nodiscard]]
auto
resolve_dwarf_line(dwarf_line_context const& context, uint8 address)
   -> source_location;

[[nodiscard]]
auto
decode_dwarf_form_size(
   span<byte const> bytes, uint8 form, idx offset_size = 4u,
   idx address_size = 8u
) -> maybe<idx>;

}  // namespace cat::detail
