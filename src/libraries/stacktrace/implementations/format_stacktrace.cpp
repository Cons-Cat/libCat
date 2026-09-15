#include <cat/array>
#include <cat/defer>
#include <cat/demangle>
#include <cat/file>
#include <cat/format>
#include <cat/linux>
#include <cat/page_allocator>
#include <cat/stacktrace>

#include "dwarf_line.hpp"

struct cat::detail::symbolizer {
   cat::span<cat::byte const> bytes;
   cat::file_path path;
   // One mapping-wide slide: runtime `__ehdr_start` minus `p_vaddr` of the
   // `PT_LOAD` at file offset 0. `ET_EXEC` is 0, since preferred VAs are runtime.
   cat::uint8 load_bias = 0u;
};

namespace {

// TODO: Eventually we need an ELF-32 ABI.

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

struct found_symbol {
   cat::str_view name;
   cat::uint8 relative_address = 0u;
};

struct resolved_frame {
   cat::detail::source_location source;
   found_symbol symbol;
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
valid_header(cat::detail::symbolizer const& symbols) -> bool {
   elf_header const* const p_header = object_at<elf_header>(symbols.bytes, 0u);
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
initialize_load_bias(cat::detail::symbolizer& symbols) -> bool {
   elf_header const& header = *object_at<elf_header>(symbols.bytes, 0u);
   if (header.type == 2u) {
      symbols.load_bias = 0u;
      return true;
   }
   if (header.type != 3u) {
      return false;
   }

   cat::uint8 const headers_size =
      cat::uint8(header.program_header_count) * sizeof(elf_program_header);
   if (!contains(symbols.bytes, header.program_header_offset, headers_size)) {
      return false;
   }

   auto const* const p_program_headers = __builtin_bit_cast(
      elf_program_header const*,
      symbols.bytes.data() + cat::idx(header.program_header_offset)
   );
   cat::uint8 const runtime_header =
      reinterpret_cast<__UINTPTR_TYPE__>(__ehdr_start);
   for (cat::idx index = 0u; index < header.program_header_count; ++index) {
      elf_program_header const& program = p_program_headers[index];
      if (program.type == 1u && program.offset == 0u) {
         symbols.load_bias = runtime_header - program.virtual_address;
         return true;
      }
   }
   return false;
}

[[nodiscard]]
auto
load_executable(cat::detail::symbolizer& symbols, cat::dyn_allocator allocator)
   -> bool {
   auto path = cat::get_executable_path(allocator);
   if (path.is_empty()) {
      return false;
   }
   symbols.path = cat::move(path).value();

   nix::scaredy_nix<nix::file_descriptor> descriptor =
      nix::sys_open(symbols.path, nix::open_mode::read_only);
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
   symbols.bytes =
      cat::span<cat::byte const>(mapping.value(), status.value().file_size);
   if (!valid_header(symbols) || !initialize_load_bias(symbols)) {
      auto _ = nix::sys_munmap(symbols.bytes);
      symbols.bytes = cat::span<cat::byte const>();
      return false;
   }

   return true;
}

[[nodiscard]]
auto
contains_runtime_address(cat::detail::symbolizer const& symbols, cat::uint8 address)
   -> bool {
   elf_header const& header = *object_at<elf_header>(symbols.bytes, 0u);
   auto const* const p_program_headers = __builtin_bit_cast(
      elf_program_header const*,
      symbols.bytes.data() + cat::idx(header.program_header_offset)
   );
   for (cat::idx index = 0u; index < header.program_header_count; ++index) {
      elf_program_header const& program = p_program_headers[index];
      cat::uint8 const begin = symbols.load_bias + program.virtual_address;
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
   cat::detail::symbolizer const& symbols, elf_section_header const& table,
   cat::uint8 relative_address
) -> found_symbol {
   elf_header const& header = *object_at<elf_header>(symbols.bytes, 0u);
   if (
      table.entry_size != sizeof(elf_symbol)
      || table.link >= header.section_header_count
      || !contains(symbols.bytes, table.offset, table.size)
   ) {
      return {};
   }

   cat::uint8 const strings_offset =
      header.section_header_offset
      + cat::uint8(table.link) * sizeof(elf_section_header);
   elf_section_header const* const p_strings_header =
      object_at<elf_section_header>(symbols.bytes, strings_offset);
   if (
      p_strings_header == nullptr
      || !contains(symbols.bytes, p_strings_header->offset, p_strings_header->size)
   ) {
      return {};
   }

   auto const* const p_symbols = __builtin_bit_cast(
      elf_symbol const*, symbols.bytes.data() + cat::idx(table.offset)
   );
   auto const* const p_strings = __builtin_bit_cast(
      char const*, symbols.bytes.data() + cat::idx(p_strings_header->offset)
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
   return {
      .name = bounded_string(
         p_strings + p_best->name,
         cat::idx(p_strings_header->size - p_best->name)
      ),
      .relative_address = p_best->value,
   };
}

[[nodiscard]]
auto
lookup_symbol(cat::detail::symbolizer const& symbols, cat::uint8 relative_address)
   -> found_symbol {
   elf_header const& header = *object_at<elf_header>(symbols.bytes, 0u);
   cat::uint8 const sections_size =
      cat::uint8(header.section_header_count) * sizeof(elf_section_header);
   if (
      header.section_header_count == 0u
      || !contains(symbols.bytes, header.section_header_offset, sections_size)
   ) {
      return {};
   }

   auto const* const p_sections = __builtin_bit_cast(
      elf_section_header const*,
      symbols.bytes.data() + cat::idx(header.section_header_offset)
   );
   for (cat::uint4 type : {2u, 11u}) {
      for (cat::idx index = 0u; index < header.section_header_count; ++index) {
         if (p_sections[index].type == type) {
            found_symbol const symbol =
               lookup_in_table(symbols, p_sections[index], relative_address);
            if (!symbol.name.is_empty()) {
               return symbol;
            }
         }
      }
   }
   return {};
}

[[nodiscard]]
auto
resolve(cat::detail::symbolizer const& symbols, cat::stacktrace_entry entry)
   -> resolved_frame {
   cat::uint8 const address = __builtin_bit_cast(cat::uint8, entry.native());
   if (symbols.bytes.is_empty() || !contains_runtime_address(symbols, address)) {
      return {};
   }
   cat::uint8 const relative_address = address - symbols.load_bias - 1u;
   return {
      .source = cat::detail::resolve_dwarf_line(symbols.bytes, relative_address),
      .symbol = lookup_symbol(symbols, relative_address),
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
      return context.append("<unknown-symbol>");
   }
   cat::maybe<cat::demangled_name> const demangled =
      cat::demangle(context.allocator.get_allocator(), symbol);
   cat::str_view const name =
      demangled.has_value() ? demangled.value().view() : symbol;
   return context.append(name).and_then([&] {
      return has_parentheses(name) ? cat::scaredy_format<void>(cat::monostate)
                                   : context.append("()");
   });
}

[[nodiscard]]
auto
decimal_width(cat::uint8 value) -> cat::idx {
   cat::idx width = 1u;
   while (value >= 10u) {
      value /= 10u;
      ++width;
   }
   return width;
}

auto
format_source_line(
   cat::uint8 line_number, cat::str_view line, cat::uint8 target,
   cat::idx width, cat::format_context& context
) -> cat::scaredy_format<void> {
   cat::scaredy_format<void> result = context.append(
      line_number == target ? cat::str_view("   > ") : cat::str_view("     ")
   );
   cat::idx const line_width = decimal_width(line_number);
   for (cat::idx index = line_width; index < width; ++index) {
      result = cat::move(result).and_then([&] {
         return context.append(' ');
      });
   }
   return cat::move(result)
      .and_then([&] {
         return cat::detail::format_nested(context, line_number);
      })
      .and_then([&] {
         return context.append(": ");
      })
      .and_then([&] {
         return context.append(line);
      })
      .and_then([&] {
         return context.append('\n');
      });
}

auto
open_source(
   cat::detail::source_location source, cat::str_view object_path,
   cat::array<char, 4'096u>& path
) -> cat::maybe<nix::file_descriptor> {
   if (source.file.size() >= path.size()) {
      return cat::nullopt;
   }
   cat::copy_memory(source.file.data(), path.data(), source.file.size());
   path[source.file.size()] = '\0';
   nix::scaredy_nix<nix::file_descriptor> opened =
      nix::sys_open(path.data(), nix::open_mode::read_only);
   if (opened.has_value()) {
      return opened.value();
   }
   if (!source.file.is_empty() && source.file[0u] == '/') {
      return cat::nullopt;
   }

   cat::idx prefix = object_path.size();
   while (prefix > 0u) {
      do {
         prefix.raw -= 1u;
      } while (prefix > 0u && object_path[prefix] != '/');
      if (prefix == 0u) {
         break;
      }
      cat::idx const path_size = prefix + 1u + source.file.size();
      if (path_size >= path.size()) {
         continue;
      }
      cat::copy_memory(object_path.data(), path.data(), prefix);
      path[prefix] = '/';
      cat::copy_memory(
         source.file.data(), path.data() + prefix.raw + 1u, source.file.size()
      );
      path[path_size] = '\0';
      opened = nix::sys_open(path.data(), nix::open_mode::read_only);
      if (opened.has_value()) {
         return opened.value();
      }
   }
   return cat::nullopt;
}

auto
format_source_snippet(
   cat::detail::source_location source, cat::str_view object_path,
   cat::format_context& context
) -> cat::scaredy_format<void> {
   cat::array<char, 4'096u> path;
   cat::maybe<nix::file_descriptor> const opened =
      open_source(source, object_path, path);
   if (opened.is_empty()) {
      return cat::monostate;
   }
   nix::file_descriptor const descriptor = opened.value();
   $defer {
      auto _ = nix::sys_close(descriptor);
   };
   cat::scaredy<nix::file_status, nix::linux_error> const status =
      nix::sys_fstat(descriptor);
   if (status.is_empty() || status.value().file_size == 0u) {
      return cat::monostate;
   }
   nix::scaredy_nix<cat::byte*> mapped = nix::sys_mmap(
      nullptr, status.value().file_size, nix::memory_protection_flags::read,
      nix::memory_flags::privately, descriptor, 0u
   );
   if (mapped.is_empty()) {
      return cat::monostate;
   }
   cat::span<cat::byte const> const bytes(
      mapped.value(), status.value().file_size
   );
   $defer {
      auto _ = nix::sys_munmap(bytes);
   };
   cat::str_view const contents(
      __builtin_bit_cast(char const*, bytes.data()), bytes.size()
   );

   cat::uint8 const first = source.line > 2u ? source.line - 2u : 1u;
   cat::uint8 const last = source.line + 2u;
   cat::idx const width = decimal_width(last);
   cat::uint8 line_number = 1u;
   cat::idx line_begin = 0u;
   for (cat::idx index = 0u; index <= contents.size(); ++index) {
      if (index != contents.size() && contents[index] != '\n') {
         continue;
      }
      if (line_number >= first && line_number <= last) {
         cat::idx line_end = index;
         if (
            line_end > line_begin && contents.data()[line_end.raw - 1u] == '\r'
         ) {
            line_end.raw -= 1u;
         }
         $prop(format_source_line(
            line_number,
            cat::str_view(
               contents.data() + line_begin.raw, cat::idx(line_end - line_begin)
            ),
            source.line, width, context
         ));
      }
      if (line_number >= last || index == contents.size()) {
         break;
      }
      ++line_number;
      line_begin = index + 1u;
   }
   return cat::monostate;
}

auto
format_short_frame(
   cat::idx index, resolved_frame resolved, cat::format_context& context
) -> cat::scaredy_format<void> {
   return context.append('#')
      .and_then([&] {
         return cat::detail::format_nested(context, index);
      })
      .and_then([&] {
         return context.append(' ');
      })
      .and_then([&] {
         if (resolved.source.line == 0u || resolved.source.file.is_empty()) {
            return context.append("<missing-file>");
         }
         return context.append(basename(resolved.source.file))
            .and_then([&] {
               return context.append(':');
            })
            .and_then([&] {
               return cat::detail::format_nested(context, resolved.source.line);
            });
      })
      .and_then([&] {
         return context.append(' ');
      })
      .and_then([&] {
         return format_signature(resolved.symbol.name, context);
      })
      .and_then([&] {
         return context.append('\n');
      });
}

auto
format_context_frame(
   cat::idx index, cat::detail::symbolizer const& symbols, resolved_frame resolved,
   cat::stacktrace_entry entry, cat::format_context& context
) -> cat::scaredy_format<void> {
   return context.append('#')
      .and_then([&] {
         return cat::detail::format_nested(context, index);
      })
      .and_then([&] {
         return context.append(" Object \"");
      })
      .and_then([&] {
         return context.append(symbols.path.view());
      })
      .and_then([&] {
         return context.append("\", at ");
      })
      .and_then([&] {
         return cat::detail::format_nested(context, entry.native());
      })
      .and_then([&] {
         return context.append(", in ");
      })
      .and_then([&] {
         return format_signature(resolved.symbol.name, context);
      })
      .and_then([&] {
         return context.append('\n');
      })
      .and_then([&] {
         if (resolved.source.line == 0u || resolved.source.file.is_empty()) {
            return cat::scaredy_format<void>(cat::monostate);
         }
         return context.append("   Source \"")
            .and_then([&] {
               return context.append(resolved.source.file);
            })
            .and_then([&] {
               return context.append("\", line ");
            })
            .and_then([&] {
               return cat::detail::format_nested(context, resolved.source.line);
            })
            .and_then([&] {
               return context.append(", in ");
            })
            .and_then([&] {
               return format_signature(resolved.symbol.name, context);
            })
            .and_then([&] {
               return context.append('\n');
            })
            .and_then([&] {
               return format_source_snippet(
                  resolved.source, symbols.path.view(), context
               );
            });
      });
}

}  // namespace

namespace cat::detail {

auto
load_symbolizer() -> symbolizer* _Nullable {
   page_allocator allocator;
   maybe const p_symbolizer = allocator.alloc<symbolizer>();
   if (p_symbolizer.is_empty()) {
      return nullptr;
   }
   // A partly loaded ELF still knows the executable's path, which is worth
   // printing even when no symbol resolves.
   auto _ = load_executable(*p_symbolizer.value(), allocator);
   return p_symbolizer.value();
}

void
unload_symbolizer(symbolizer* _Nullable p_symbolizer) {
   if (p_symbolizer == nullptr) {
      return;
   }
   page_allocator allocator;
   auto _ = nix::sys_munmap(p_symbolizer->bytes);
   p_symbolizer->path.free(allocator);
   allocator.free(p_symbolizer);
}

auto
resolve_stacktrace_frame(
   symbolizer const* _Nullable p_symbolizer, stacktrace_entry entry
) -> frame_origin {
   if (p_symbolizer == nullptr) {
      return {};
   }
   resolved_frame const resolved = resolve(*p_symbolizer, entry);
   frame_origin origin{
      .file = resolved.source.file,
      .line = resolved.source.line,
      .symbol = resolved.symbol.name,
   };
   if (!resolved.symbol.name.is_empty()) {
      origin.p_function = __builtin_bit_cast(
         void*, resolved.symbol.relative_address + p_symbolizer->load_bias
      );
   }
   return origin;
}

auto
format_stacktrace_frame(
   symbolizer const* _Nullable p_symbolizer, idx number, stacktrace_entry entry,
   format_context& context, bool debug
) -> scaredy_format<void> {
   symbolizer const unloaded{};
   symbolizer const& symbols = p_symbolizer != nullptr ? *p_symbolizer : unloaded;
   resolved_frame const resolved = resolve(symbols, entry);
   if (debug) {
      return format_context_frame(number, symbols, resolved, entry, context);
   }
   return format_short_frame(number, resolved, context);
}

auto
format_stacktrace_entry_context(stacktrace_entry entry, format_context& context)
   -> scaredy_format<void> {
   symbolizer* _Nullable const p_symbolizer = load_symbolizer();
   scaredy_format<void> const result =
      format_stacktrace_frame(p_symbolizer, 1u, entry, context, true);
   unload_symbolizer(p_symbolizer);
   return result;
}

}  // namespace cat::detail
