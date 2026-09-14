#include "dwarf_line.hpp"

#include <cat/array>
#include <cat/bit>
#include <cat/span>
#include <cat/string>

namespace {

struct elf_header {
   cat::array<cat::uint1, 16u> identification;
   cat::uint2 type;
   cat::uint2 machine;
   cat::uint4 version;
   cat::uint8 entry;
   cat::uint8 program_header_offset;
   cat::uint8 section_header_offset;
   cat::uint4 flags;
   cat::uint2 header_size;
   cat::uint2 program_header_size;
   cat::uint2 program_header_count;
   cat::uint2 section_header_size;
   cat::uint2 section_header_count;
   cat::uint2 section_name_index;
};

struct elf_section_header {
   cat::uint4 name;
   cat::uint4 type;
   cat::uint8 flags;
   cat::uint8 address;
   cat::uint8 offset;
   cat::uint8 size;
   cat::uint4 link;
   cat::uint4 info;
   cat::uint8 alignment;
   cat::uint8 entry_size;
};

static_assert(sizeof(elf_header) == 64);
static_assert(sizeof(elf_section_header) == 64);

struct dwarf_sections {
   cat::span<cat::byte const> line;
   cat::span<cat::byte const> line_strings;
   cat::span<cat::byte const> strings;
};

struct line_header {
   cat::uint2 version = 0u;
   cat::uint1 address_size = 8u;
   cat::uint1 minimum_instruction_length = 0u;
   cat::uint1 maximum_operations = 1u;
   cat::int1 line_base = 0;
   cat::uint1 line_range = 0u;
   cat::uint1 opcode_base = 0u;
   cat::idx standard_lengths = 0u;
   cat::idx program = 0u;
   cat::idx unit_end = 0u;
   cat::idx files = 0u;
   cat::idx file_count = 0u;
   cat::idx file_formats = 0u;
   cat::idx file_formats_end = 0u;
   cat::idx file_format_count = 0u;
   cat::idx offset_size = 4u;
   cat::idx old_files = 0u;
};

struct line_row {
   cat::uint8 address = 0u;
   cat::uint8 line = 1u;
   cat::uint8 file = 1u;
   cat::uint8 operation = 0u;
};

struct reader {
   cat::span<cat::byte const> bytes;
   cat::idx position = 0u;
   cat::idx limit = 0u;

   explicit reader(cat::span<cat::byte const> const& input)
       : bytes(input), limit(input.size()) {
   }

   reader(cat::span<cat::byte const> const& input, cat::idx begin, cat::idx end)
       : bytes(input), position(begin), limit(end) {
   }

   [[nodiscard]]
   auto
   read_unsigned(cat::idx size, cat::uint8& value) -> bool {
      if (size > limit - position) {
         return false;
      }
      value = 0u;
      for (cat::idx index = 0u; index < size; ++index) {
         value |= cat::uint8(bytes[position + index].value) << (index.raw * 8u);
      }
      position += size;
      return true;
   }

   [[nodiscard]]
   auto
   read_u1(cat::uint1& value) -> bool {
      cat::uint8 wide = 0u;
      if (!read_unsigned(1u, wide)) {
         return false;
      }
      value = cat::uint1(wide);
      return true;
   }

   [[nodiscard]]
   auto
   read_u2(cat::uint2& value) -> bool {
      cat::uint8 wide = 0u;
      if (!read_unsigned(2u, wide)) {
         return false;
      }
      value = cat::uint2(wide);
      return true;
   }

   [[nodiscard]]
   auto
   read_u4(cat::uint4& value) -> bool {
      cat::uint8 wide = 0u;
      if (!read_unsigned(4u, wide)) {
         return false;
      }
      value = cat::uint4(wide);
      return true;
   }

   [[nodiscard]]
   auto
   read_uleb(cat::uint8& value) -> bool {
      value = 0u;
      cat::uint1 shift = 0u;
      while (position < limit && shift < 64u) {
         cat::uint1 const part = bytes[position].value;
         ++position;
         value |= cat::uint8(part & 0x7fu) << shift;
         if ((part & 0x80u) == 0u) {
            return true;
         }
         shift += 7u;
      }
      return false;
   }

   [[nodiscard]]
   auto
   read_sleb(cat::int8& value) -> bool {
      cat::uint8 result = 0u;
      cat::uint1 shift = 0u;
      cat::uint1 part = 0u;
      do {
         if (position >= limit || shift >= 64u) {
            return false;
         }
         part = bytes[position].value;
         ++position;
         result |= cat::uint8(part & 0x7fu) << shift;
         shift += 7u;
      } while ((part & 0x80u) != 0u);
      if (shift < 64u && (part & 0x40u) != 0u) {
         result |= cat::uint8::max() << shift;
      }
      value = __builtin_bit_cast(cat::int8, result);
      return true;
   }

   [[nodiscard]]
   auto
   skip(cat::uint8 size) -> bool {
      if (size > limit - position) {
         return false;
      }
      position += cat::idx(size);
      return true;
   }

   [[nodiscard]]
   auto
   read_string(cat::str_view& value) -> bool {
      cat::idx const begin = position;
      while (position < limit && bytes[position] != 0u) {
         ++position;
      }
      if (position == limit) {
         return false;
      }
      value = cat::str_view(
         __builtin_bit_cast(char const*, bytes.data() + begin),
         cat::idx(position.raw - begin.raw)
      );
      ++position;
      return true;
   }
};

[[nodiscard]]
auto
contains(cat::span<cat::byte const> bytes, cat::uint8 offset, cat::uint8 size)
   -> bool {
   return offset <= bytes.size() && size <= bytes.size() - cat::idx(offset);
}

[[nodiscard]]
auto
bounded_string(cat::span<cat::byte const> bytes, cat::uint8 offset)
   -> cat::str_view {
   if (offset >= bytes.size()) {
      return {};
   }
   reader input(bytes, cat::idx(offset), bytes.size());
   cat::str_view result;
   if (!input.read_string(result)) {
      return {};
   }
   return result;
}

[[nodiscard]]
auto
section_name(
   cat::span<cat::byte const> image, elf_section_header const& strings,
   cat::uint4 offset
) -> cat::str_view {
   if (
      offset >= strings.size || !contains(image, strings.offset, strings.size)
   ) {
      return {};
   }
   return bounded_string(image, strings.offset + offset);
}

[[nodiscard]]
auto
find_sections(cat::span<cat::byte const> image) -> dwarf_sections {
   if (image.size() < sizeof(elf_header)) {
      return {};
   }
   auto const* const p_header =
      __builtin_bit_cast(elf_header const*, image.data());
   cat::uint8 const sections_size =
      cat::uint8(p_header->section_header_count) * sizeof(elf_section_header);
   if (
      p_header->section_header_size != sizeof(elf_section_header)
      || p_header->section_name_index >= p_header->section_header_count
      || !contains(image, p_header->section_header_offset, sections_size)
   ) {
      return {};
   }
   auto const* const p_sections = __builtin_bit_cast(
      elf_section_header const*,
      image.data() + cat::idx(p_header->section_header_offset)
   );
   elf_section_header const& names =
      p_sections[cat::idx(p_header->section_name_index).raw];
   dwarf_sections result;
   for (cat::idx index = 0u; index < p_header->section_header_count; ++index) {
      elf_section_header const& section = p_sections[index];
      if (!contains(image, section.offset, section.size)) {
         continue;
      }
      cat::str_view const name = section_name(image, names, section.name);
      cat::span<cat::byte const> const bytes(
         image.data() + cat::idx(section.offset), cat::idx(section.size)
      );
      if (name == ".debug_line") {
         result.line = bytes;
      } else if (name == ".debug_line_str") {
         result.line_strings = bytes;
      } else if (name == ".debug_str") {
         result.strings = bytes;
      }
   }
   return result;
}

[[nodiscard]]
auto
read_offset(reader& input, cat::idx size, cat::uint8& value) -> bool {
   return input.read_unsigned(size, value);
}

[[nodiscard]]
auto
skip_block(reader& input, cat::idx length_size) -> bool {
   cat::uint8 length = 0u;
   return input.read_unsigned(length_size, length) && input.skip(length);
}

[[nodiscard]]
auto
skip_uleb_block(reader& input) -> bool {
   cat::uint8 length = 0u;
   return input.read_uleb(length) && input.skip(length);
}

[[nodiscard]]
auto
skip_form(
   reader& input, cat::uint8 form, cat::idx offset_size, cat::idx address_size
) -> bool {
   cat::uint8 value = 0u;
   switch (form.raw) {
      case 0x01u:
         return input.skip(address_size);
      case 0x03u:
         return skip_block(input, 2u);
      case 0x04u:
         return skip_block(input, 4u);
      case 0x05u:
         return input.skip(2u);
      case 0x06u:
         return input.skip(4u);
      case 0x07u:
         return input.skip(8u);
      case 0x08u:
         {
            cat::str_view ignored;
            return input.read_string(ignored);
         }
      case 0x09u:
      case 0x18u:
         return skip_uleb_block(input);
      case 0x0au:
         return skip_block(input, 1u);
      case 0x0bu:
      case 0x0cu:
         return input.skip(1u);
      case 0x0du:
         {
            cat::int8 signed_value = 0;
            return input.read_sleb(signed_value);
         }
      case 0x0eu:
      case 0x17u:
      case 0x1du:
      case 0x1fu:
         return input.skip(offset_size);
      case 0x0fu:
      case 0x1au:
      case 0x1bu:
      case 0x22u:
      case 0x23u:
         return input.read_uleb(value);
      case 0x1eu:
         return input.skip(16u);
      case 0x19u:
      case 0x21u:
         return true;
      case 0x20u:
         return input.skip(8u);
      case 0x25u:
      case 0x29u:
         return input.skip(1u);
      case 0x26u:
      case 0x2au:
         return input.skip(2u);
      case 0x27u:
      case 0x2bu:
         return input.skip(3u);
      case 0x28u:
      case 0x2cu:
         return input.skip(4u);
      default:
         return false;
   }
}

[[nodiscard]]
auto
read_format(reader& input, cat::uint8& content, cat::uint8& form) -> bool {
   if (!input.read_uleb(content) || !input.read_uleb(form)) {
      return false;
   }
   if (form == 0x21u) {
      cat::int8 implicit_value = 0;
      return input.read_sleb(implicit_value);
   }
   return true;
}

[[nodiscard]]
auto
read_form_string(
   reader& input, cat::uint8 form, cat::idx offset_size,
   dwarf_sections const& sections, cat::str_view& value
) -> bool {
   if (form == 0x08u) {
      return input.read_string(value);
   }
   cat::uint8 offset = 0u;
   if (
      (form != 0x0eu && form != 0x1fu)
      || !read_offset(input, offset_size, offset)
   ) {
      return false;
   }
   value = bounded_string(
      form == 0x1fu ? sections.line_strings : sections.strings, offset
   );
   return !value.is_empty();
}

[[nodiscard]]
auto
parse_v5_table(
   reader& input, cat::idx offset_size, cat::idx address_size,
   line_header& header
) -> bool {
   cat::uint1 directory_format_count = 0u;
   if (!input.read_u1(directory_format_count)) {
      return false;
   }
   cat::idx const directory_formats = input.position;
   for (cat::idx index = 0u; index < directory_format_count; ++index) {
      cat::uint8 content = 0u;
      cat::uint8 form = 0u;
      if (!read_format(input, content, form)) {
         return false;
      }
   }
   cat::idx const directory_entries = input.position;
   cat::uint8 directory_count = 0u;
   if (!input.read_uleb(directory_count)) {
      return false;
   }
   for (cat::uint8 entry = 0u; entry < directory_count; ++entry) {
      reader formats(input.bytes, directory_formats, directory_entries);
      for (cat::idx index = 0u; index < directory_format_count; ++index) {
         cat::uint8 content = 0u;
         cat::uint8 form = 0u;
         if (
            !read_format(formats, content, form)
            || !skip_form(input, form, offset_size, address_size)
         ) {
            return false;
         }
      }
   }

   cat::uint1 file_format_count = 0u;
   if (!input.read_u1(file_format_count)) {
      return false;
   }
   header.file_formats = input.position;
   header.file_format_count = file_format_count;
   for (cat::idx index = 0u; index < file_format_count; ++index) {
      cat::uint8 content = 0u;
      cat::uint8 form = 0u;
      if (!read_format(input, content, form)) {
         return false;
      }
   }
   cat::idx const formats_end = input.position;
   header.file_formats_end = formats_end;
   cat::uint8 file_count = 0u;
   if (!input.read_uleb(file_count)) {
      return false;
   }
   header.files = input.position;
   header.file_count = cat::idx(file_count);
   for (cat::uint8 entry = 0u; entry < file_count; ++entry) {
      reader formats(input.bytes, header.file_formats, formats_end);
      for (cat::idx index = 0u; index < file_format_count; ++index) {
         cat::uint8 content = 0u;
         cat::uint8 form = 0u;
         if (
            !read_format(formats, content, form)
            || !skip_form(input, form, offset_size, address_size)
         ) {
            return false;
         }
      }
   }
   return true;
}

[[nodiscard]]
auto
parse_old_table(reader& input, line_header& header) -> bool {
   cat::str_view entry;
   do {
      if (!input.read_string(entry)) {
         return false;
      }
   } while (!entry.is_empty());
   header.old_files = input.position;
   while (true) {
      if (!input.read_string(entry)) {
         return false;
      }
      if (entry.is_empty()) {
         return true;
      }
      cat::uint8 ignored = 0u;
      if (
         !input.read_uleb(ignored) || !input.read_uleb(ignored)
         || !input.read_uleb(ignored)
      ) {
         return false;
      }
      ++header.file_count;
   }
}

[[nodiscard]]
auto
parse_header(
   dwarf_sections const& sections, cat::idx unit_start, line_header& header,
   cat::idx& next_unit
) -> bool {
   reader input(sections.line, unit_start, sections.line.size());
   cat::uint4 initial_length = 0u;
   if (!input.read_u4(initial_length)) {
      return false;
   }
   cat::idx offset_size = 4u;
   cat::uint8 unit_length = initial_length;
   if (initial_length == cat::uint4::max()) {
      offset_size = 8u;
      if (!input.read_unsigned(8u, unit_length)) {
         return false;
      }
   } else if (initial_length >= 0xfffffff0u) {
      return false;
   }
   header.offset_size = offset_size;
   if (unit_length > input.limit - input.position) {
      return false;
   }
   header.unit_end = input.position + cat::idx(unit_length);
   next_unit = header.unit_end;
   input.limit = header.unit_end;
   if (
      !input.read_u2(header.version) || header.version < 2u
      || header.version > 5u
   ) {
      return false;
   }
   if (header.version == 5u) {
      cat::uint1 segment_size = 0u;
      if (
         !input.read_u1(header.address_size) || !input.read_u1(segment_size)
         || segment_size != 0u
      ) {
         return false;
      }
   }
   cat::uint8 header_length = 0u;
   if (!read_offset(input, offset_size, header_length)) {
      return false;
   }
   cat::idx const header_end = input.position + cat::idx(header_length);
   if (header_end > input.limit) {
      return false;
   }
   cat::uint1 default_statement = 0u;
   cat::uint1 raw_line_base = 0u;
   if (!input.read_u1(header.minimum_instruction_length)) {
      return false;
   }
   if (header.version >= 4u && !input.read_u1(header.maximum_operations)) {
      return false;
   }
   if (
      header.maximum_operations == 0u || !input.read_u1(default_statement)
      || !input.read_u1(raw_line_base) || !input.read_u1(header.line_range)
      || !input.read_u1(header.opcode_base) || header.line_range == 0u
      || header.opcode_base == 0u
   ) {
      return false;
   }
   header.line_base = __builtin_bit_cast(cat::int1, raw_line_base);
   header.standard_lengths = input.position;
   if (!input.skip(header.opcode_base - 1u)) {
      return false;
   }
   bool const valid_table =
      header.version == 5u
         ? parse_v5_table(input, offset_size, header.address_size, header)
         : parse_old_table(input, header);
   if (!valid_table || input.position != header_end) {
      return false;
   }
   header.program = header_end;
   return true;
}

[[nodiscard]]
auto
v5_file(
   dwarf_sections const& sections, line_header const& header,
   cat::uint8 file_index
) -> cat::str_view {
   if (file_index >= header.file_count) {
      return {};
   }
   reader input(sections.line, header.files, header.program);
   for (cat::uint8 entry = 0u; entry <= file_index; ++entry) {
      reader formats(
         sections.line, header.file_formats, header.file_formats_end
      );
      cat::str_view result;
      for (cat::idx index = 0u; index < header.file_format_count; ++index) {
         cat::uint8 content = 0u;
         cat::uint8 form = 0u;
         if (!read_format(formats, content, form)) {
            return {};
         }
         if (content == 1u) {
            cat::str_view path;
            if (!read_form_string(
                   input, form, header.offset_size, sections, path
                )) {
               return {};
            }
            if (entry == file_index) {
               result = path;
            }
         } else if (!skip_form(
                       input, form, header.offset_size, header.address_size
                    )) {
            return {};
         }
      }
      if (entry == file_index) {
         return result;
      }
   }
   return {};
}

[[nodiscard]]
auto
old_file(
   dwarf_sections const& sections, line_header const& header,
   cat::uint8 file_index
) -> cat::str_view {
   if (file_index == 0u || file_index > header.file_count) {
      return {};
   }
   reader input(sections.line, header.old_files, header.program);
   for (cat::uint8 entry = 1u; entry <= file_index; ++entry) {
      cat::str_view path;
      cat::uint8 ignored = 0u;
      if (
         !input.read_string(path) || path.is_empty()
         || !input.read_uleb(ignored) || !input.read_uleb(ignored)
         || !input.read_uleb(ignored)
      ) {
         return {};
      }
      if (entry == file_index) {
         return path;
      }
   }
   return {};
}

[[nodiscard]]
auto
file_name(
   dwarf_sections const& sections, line_header const& header,
   cat::uint8 file_index
) -> cat::str_view {
   return header.version == 5u ? v5_file(sections, header, file_index)
                               : old_file(sections, header, file_index);
}

void
advance_address(
   line_row& row, line_header const& header, cat::uint8 operations
) {
   cat::uint8 const combined = row.operation + operations;
   row.address += cat::uint8(header.minimum_instruction_length)
                  * (combined / header.maximum_operations);
   row.operation = combined % header.maximum_operations;
}

[[nodiscard]]
auto
matches_row(
   dwarf_sections const& sections, line_header const& header,
   line_row const& previous, line_row const& current, cat::uint8 address,
   bool previous_valid
) -> cat::detail::source_location {
   if (
      previous_valid && address >= previous.address && address < current.address
   ) {
      return {
         .file = file_name(sections, header, previous.file),
         .line = previous.line,
      };
   }
   return {};
}

[[nodiscard]]
auto
run_program(
   dwarf_sections const& sections, line_header const& header, cat::uint8 address
) -> cat::detail::source_location {
   reader input(sections.line, header.program, header.unit_end);
   line_row row;
   line_row previous;
   bool previous_valid = false;
   while (input.position < input.limit) {
      cat::uint1 opcode = 0u;
      if (!input.read_u1(opcode)) {
         return {};
      }
      bool emit = false;
      bool end_sequence = false;
      if (opcode == 0u) {
         cat::uint8 length = 0u;
         if (
            !input.read_uleb(length) || length == 0u
            || length > input.limit - input.position
         ) {
            return {};
         }
         cat::idx const extended_end = input.position + cat::idx(length);
         cat::uint1 extended = 0u;
         if (!input.read_u1(extended)) {
            return {};
         }
         if (extended == 1u) {
            emit = true;
            end_sequence = true;
         } else if (extended == 2u) {
            cat::uint8 new_address = 0u;
            if (!input.read_unsigned(
                   cat::idx(extended_end.raw - input.position.raw), new_address
                )) {
               return {};
            }
            row.address = new_address;
            row.operation = 0u;
         }
         input.position = extended_end;
      } else if (opcode < header.opcode_base) {
         cat::uint8 operand = 0u;
         cat::int8 signed_operand = 0;
         switch (opcode.raw) {
            case 1u:
               emit = true;
               break;
            case 2u:
               if (!input.read_uleb(operand)) {
                  return {};
               }
               advance_address(row, header, operand);
               break;
            case 3u:
               if (!input.read_sleb(signed_operand)) {
                  return {};
               }
               row.line = cat::uint8(cat::int8(row.line) + signed_operand);
               break;
            case 4u:
               if (!input.read_uleb(row.file)) {
                  return {};
               }
               break;
            case 5u:
            case 12u:
               if (!input.read_uleb(operand)) {
                  return {};
               }
               break;
            case 8u:
               advance_address(
                  row, header, (255u - header.opcode_base) / header.line_range
               );
               break;
            case 9u:
               {
                  cat::uint2 fixed = 0u;
                  if (!input.read_u2(fixed)) {
                     return {};
                  }
                  row.address += fixed;
                  row.operation = 0u;
                  break;
               }
            default:
               {
                  reader lengths(
                     sections.line, header.standard_lengths, header.program
                  );
                  lengths.position += opcode - 1u;
                  cat::uint1 operand_count = 0u;
                  if (!lengths.read_u1(operand_count)) {
                     return {};
                  }
                  for (cat::idx index = 0u; index < operand_count; ++index) {
                     if (!input.read_uleb(operand)) {
                        return {};
                     }
                  }
                  break;
               }
         }
      } else {
         cat::uint8 const adjusted = opcode - header.opcode_base;
         advance_address(row, header, adjusted / header.line_range);
         row.line = cat::uint8(
            cat::int8(row.line) + header.line_base
            + cat::int8(adjusted % header.line_range)
         );
         emit = true;
      }
      if (emit) {
         cat::detail::source_location const found = matches_row(
            sections, header, previous, row, address, previous_valid
         );
         if (found.line != 0u) {
            return found;
         }
         if (end_sequence) {
            row = {};
            row.line = 1u;
            row.file = 1u;
            previous_valid = false;
         } else {
            previous = row;
            previous_valid = true;
         }
      }
   }
   return {};
}

}  // namespace

auto
cat::detail::decode_dwarf_form_size(
   span<byte const> bytes, uint8 form, idx offset_size, idx address_size
) -> maybe<idx> {
   reader input(bytes);
   if (!skip_form(input, form, offset_size, address_size)) {
      return {};
   }
   return input.position;
}

auto
cat::detail::resolve_dwarf_line(span<byte const> image, uint8 address)
   -> source_location {
   dwarf_sections const sections = find_sections(image);
   if (sections.line.is_empty()) {
      return {};
   }
   for (idx offset = 0u; offset < sections.line.size();) {
      line_header header;
      idx next = sections.line.size();
      if (!parse_header(sections, offset, header, next)) {
         if (next <= offset) {
            return {};
         }
         offset = next;
         continue;
      }
      source_location const result = run_program(sections, header, address);
      if (result.line != 0u) {
         return result;
      }
      offset = next;
   }
   return {};
}
