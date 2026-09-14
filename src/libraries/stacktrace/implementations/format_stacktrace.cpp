#include <cat/array>
#include <cat/defer>
#include <cat/demangle>
#include <cat/format>
#include <cat/linux>
#include <cat/page_allocator>
#include <cat/stacktrace>

#include "dwarf_line.hpp"

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

struct elf_program_header {
   cat::uint4 type;
   cat::uint4 flags;
   cat::uint8 offset;
   cat::uint8 virtual_address;
   cat::uint8 physical_address;
   cat::uint8 file_size;
   cat::uint8 memory_size;
   cat::uint8 alignment;
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

struct elf_symbol {
   cat::uint4 name;
   unsigned char info;
   unsigned char other;
   cat::uint2 section_index;
   cat::uint8 value;
   cat::uint8 size;
};

static_assert(sizeof(elf_header) == 64);
static_assert(sizeof(elf_program_header) == 56);
static_assert(sizeof(elf_section_header) == 64);
static_assert(sizeof(elf_symbol) == 24);

extern "C" char __ehdr_start[];

struct executable_image {
   cat::span<cat::byte const> bytes;
   cat::uint8 load_bias = 0u;
};

struct resolved_frame {
   cat::detail::source_location source;
   cat::str_view symbol;
};

[[nodiscard]]
auto
contains(cat::span<cat::byte const> bytes, cat::uint8 offset, cat::uint8 size)
   -> bool {
   return offset <= bytes.size() && size <= bytes.size() - cat::idx(offset);
}

template <typename T>
[[nodiscard]]
auto
object_at(cat::span<cat::byte const> bytes, cat::uint8 offset)
   -> T const* _Nullable {
   if (!contains(bytes, offset, sizeof(T))) {
      return nullptr;
   }
   return __builtin_bit_cast(T const*, bytes.data() + cat::idx(offset));
}

[[nodiscard]]
auto
valid_header(executable_image const& image) -> bool {
   elf_header const* const p_header = object_at<elf_header>(image.bytes, 0u);
   return p_header != nullptr && p_header->identification[0] == 0x7f
          && p_header->identification[1] == 'E'
          && p_header->identification[2] == 'L'
          && p_header->identification[3] == 'F'
          && p_header->identification[4] == 2
          && p_header->identification[5] == 1
          && p_header->identification[6] == 1
          && p_header->program_header_size == sizeof(elf_program_header)
          && p_header->section_header_size == sizeof(elf_section_header);
}

[[nodiscard]]
auto
initialize_load_bias(executable_image& image) -> bool {
   elf_header const& header = *object_at<elf_header>(image.bytes, 0u);
   if (header.type == 2u) {
      image.load_bias = 0u;
      return true;
   }
   if (header.type != 3u) {
      return false;
   }

   cat::uint8 const headers_size =
      cat::uint8(header.program_header_count) * sizeof(elf_program_header);
   if (!contains(image.bytes, header.program_header_offset, headers_size)) {
      return false;
   }

   auto const* const p_program_headers = __builtin_bit_cast(
      elf_program_header const*,
      image.bytes.data() + cat::idx(header.program_header_offset)
   );
   cat::uint8 const runtime_header =
      reinterpret_cast<__UINTPTR_TYPE__>(__ehdr_start);
   for (cat::idx index = 0u; index < header.program_header_count; ++index) {
      elf_program_header const& program = p_program_headers[index];
      if (program.type == 1u && program.offset == 0u) {
         image.load_bias = runtime_header - program.virtual_address;
         return true;
      }
   }
   return false;
}

[[nodiscard]]
auto
load_executable(executable_image& image) -> bool {
   nix::scaredy_nix<nix::file_descriptor> descriptor =
      nix::sys_open("/proc/self/exe", nix::open_mode::read_only);
   if (descriptor.is_empty()) {
      return false;
   }
   $defer {
      auto _ = nix::sys_close(descriptor.value());
   };

   cat::scaredy<nix::file_status, nix::linux_error> const status =
      nix::sys_fstat(descriptor.value());
   if (status.is_empty() || status.value().file_size == 0u) {
      return false;
   }

   nix::scaredy_nix<cat::byte*> mapping = nix::sys_mmap(
      nullptr, status.value().file_size, nix::memory_protection_flags::read,
      nix::memory_flags::privately, descriptor.value(), 0u
   );
   if (mapping.is_empty()) {
      return false;
   }
   image.bytes =
      cat::span<cat::byte const>(mapping.value(), status.value().file_size);
   if (!valid_header(image) || !initialize_load_bias(image)) {
      auto _ = nix::sys_munmap(image.bytes);
      image.bytes = cat::span<cat::byte const>();
      return false;
   }

   return true;
}

[[nodiscard]]
auto
contains_runtime_address(executable_image const& image, cat::uint8 address)
   -> bool {
   elf_header const& header = *object_at<elf_header>(image.bytes, 0u);
   auto const* const p_program_headers = __builtin_bit_cast(
      elf_program_header const*,
      image.bytes.data() + cat::idx(header.program_header_offset)
   );
   for (cat::idx index = 0u; index < header.program_header_count; ++index) {
      elf_program_header const& program = p_program_headers[index];
      cat::uint8 const begin = image.load_bias + program.virtual_address;
      if (
         program.type == 1u && address >= begin
         && address - begin < program.memory_size
      ) {
         return true;
      }
   }
   return false;
}

[[nodiscard]]
auto
bounded_string(char const* _Nonnull p_string, cat::idx maximum_size)
   -> cat::str_view {
   cat::idx length = 0u;
   while (length < maximum_size && p_string[length.raw] != '\0') {
      ++length;
   }
   return {p_string, length};
}

[[nodiscard]]
auto
lookup_in_table(
   executable_image const& image, elf_section_header const& table,
   cat::uint8 relative_address
) -> cat::str_view {
   elf_header const& header = *object_at<elf_header>(image.bytes, 0u);
   if (
      table.entry_size != sizeof(elf_symbol)
      || table.link >= header.section_header_count
      || !contains(image.bytes, table.offset, table.size)
   ) {
      return {};
   }

   cat::uint8 const strings_offset =
      header.section_header_offset
      + cat::uint8(table.link) * sizeof(elf_section_header);
   elf_section_header const* const p_strings_header =
      object_at<elf_section_header>(image.bytes, strings_offset);
   if (
      p_strings_header == nullptr
      || !contains(
         image.bytes, p_strings_header->offset, p_strings_header->size
      )
   ) {
      return {};
   }

   auto const* const p_symbols = __builtin_bit_cast(
      elf_symbol const*, image.bytes.data() + cat::idx(table.offset)
   );
   auto const* const p_strings = __builtin_bit_cast(
      char const*, image.bytes.data() + cat::idx(p_strings_header->offset)
   );
   cat::idx const symbol_count = cat::idx(table.size / sizeof(elf_symbol));
   elf_symbol const* p_best = nullptr;
   for (cat::idx index = 0u; index < symbol_count; ++index) {
      elf_symbol const& symbol = p_symbols[index];
      if (
         (symbol.info & 0xfu) != 2u || symbol.section_index == 0u
         || symbol.name >= p_strings_header->size
         || symbol.value > relative_address
      ) {
         continue;
      }
      if (
         (symbol.size == 0u && symbol.value != relative_address)
         || (symbol.size != 0u && relative_address - symbol.value >= symbol.size)
      ) {
         continue;
      }
      if (p_best == nullptr || symbol.value > p_best->value) {
         p_best = &symbol;
      }
   }
   if (p_best == nullptr) {
      return {};
   }
   return bounded_string(
      p_strings + p_best->name, cat::idx(p_strings_header->size - p_best->name)
   );
}

[[nodiscard]]
auto
lookup_symbol(executable_image const& image, cat::uint8 relative_address)
   -> cat::str_view {
   elf_header const& header = *object_at<elf_header>(image.bytes, 0u);
   cat::uint8 const sections_size =
      cat::uint8(header.section_header_count) * sizeof(elf_section_header);
   if (
      header.section_header_count == 0u
      || !contains(image.bytes, header.section_header_offset, sections_size)
   ) {
      return {};
   }

   auto const* const p_sections = __builtin_bit_cast(
      elf_section_header const*,
      image.bytes.data() + cat::idx(header.section_header_offset)
   );
   for (cat::uint4 type : {2u, 11u}) {
      for (cat::idx index = 0u; index < header.section_header_count; ++index) {
         if (p_sections[index].type == type) {
            cat::str_view const symbol =
               lookup_in_table(image, p_sections[index], relative_address);
            if (!symbol.is_empty()) {
               return symbol;
            }
         }
      }
   }
   return {};
}

[[nodiscard]]
auto
resolve(executable_image const& image, cat::stacktrace_entry entry)
   -> resolved_frame {
   cat::uint8 const address = __builtin_bit_cast(cat::uint8, entry.native());
   if (image.bytes.is_empty() || !contains_runtime_address(image, address)) {
      return {};
   }
   cat::uint8 const relative_address = address - image.load_bias - 1u;
   return {
      .source = cat::detail::resolve_dwarf_line(image.bytes, relative_address),
      .symbol = lookup_symbol(image, relative_address),
   };
}

[[nodiscard]]
auto
basename(cat::str_view path) -> cat::str_view {
   cat::idx begin = 0u;
   for (cat::idx index = 0u; index < path.size(); ++index) {
      if (path[index] == '/') {
         begin = index + 1u;
      }
   }
   return path.remove_prefix(begin);
}

[[nodiscard]]
auto
has_parentheses(cat::str_view name) -> bool {
   return name.find('(').has_value();
}

auto
format_signature(cat::str_view symbol, cat::format_context& context)
   -> cat::scaredy_format<void> {
   if (symbol.is_empty()) {
      return context.append("<unknown>()");
   }
   cat::maybe<cat::demangled_name> const demangled =
      cat::demangle(context.allocator.get_allocator(), symbol);
   cat::str_view const name =
      demangled.has_value() ? demangled.value().view() : symbol;
   $prop(context.append(name));
   if (!has_parentheses(name)) {
      $prop(context.append("()"));
   }
   return cat::monostate;
}

auto
format_frame(
   cat::idx index, resolved_frame resolved, cat::format_context& context
) -> cat::scaredy_format<void> {
   $prop(context.append('#'));
   $prop(cat::detail::format_nested(context, index));
   $prop(context.append(' '));
   if (resolved.source.line != 0u && !resolved.source.file.is_empty()) {
      $prop(context.append(basename(resolved.source.file)));
      $prop(context.append(':'));
      $prop(cat::detail::format_nested(context, resolved.source.line));
   } else {
      $prop(context.append("<unknown>"));
   }
   $prop(context.append(' '));
   $prop(format_signature(resolved.symbol, context));
   return context.append('\n');
}

}  // namespace

auto
cat::detail::format_stacktrace(stacktrace const& trace, format_context& context)
   -> scaredy_format<void> {
   $prop(context.append("Stack trace:\n"));

   executable_image image;
   if (load_executable(image)) {
      $defer {
         auto _ = nix::sys_munmap(image.bytes);
      };
      for (idx index = 0u; index < trace.size(); ++index) {
         $prop(format_frame(index + 1u, resolve(image, trace[index]), context));
      }
      return monostate;
   }

   for (idx index = 0u; index < trace.size(); ++index) {
      $prop(format_frame(index + 1u, {}, context));
   }
   return monostate;
}

namespace cat::detail {

[[gnu::noinline, clang::disable_tail_calls]]
void
print_failing_stacktrace() {
   page_allocator allocator;
   maybe<stacktrace> const trace = stacktrace::current(allocator, 3u);
   if (trace.is_empty()) {
      eprint("Stack trace is unavailable.\n").or_exit();
      return;
   }
   eprint_fmt(allocator, "{}", trace.value()).or_exit();
}

}  // namespace cat::detail
