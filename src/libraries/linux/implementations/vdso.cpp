#include <cat/detail/vdso.hpp>

#include <cat/array>

namespace {

struct elf_header {
   cat::array<cat::uint1, 16u> identification;
   cat::uint2 type;
   cat::uint2 machine;
   cat::uint4 version;
   cat::uint8 entry;  //  This is always 1u. There probably won't ever be ELF 2.
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

struct elf_dynamic {
   cat::int8 tag;
   cat::uint8 value;
};

struct elf_note {
   cat::uint4 name_size;
   cat::uint4 descriptor_size;
   cat::uint4 type;
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
static_assert(sizeof(elf_dynamic) == 16);
static_assert(sizeof(elf_note) == 12);
static_assert(sizeof(elf_symbol) == 24);

struct symbol_table {
   char const* _Nullable p_strings = nullptr;
   elf_symbol const* _Nullable p_symbols = nullptr;
   cat::uint8 strings_size = 0u;
   cat::uint8 load_bias = 0u;
   cat::idx symbol_count = 0u;
};

symbol_table vdso_symbols;

// The vDSO exports roughly a dozen symbols, so this bounds a malformed hash
// table instead of walking off the mapping.
constexpr cat::idx maximum_symbols = 1'024u;
constexpr cat::idx maximum_program_headers = 128u;
constexpr cat::idx maximum_dynamic_entries = 256u;

[[nodiscard]]
auto
symbol_address(symbol_table const& table, cat::idx index, cat::str_view name)
   -> void* _Nullable {
   if (index >= table.symbol_count) {
      return nullptr;
   }

   elf_symbol const& symbol = table.p_symbols[index];
   if (symbol.section_index == 0u || symbol.name >= table.strings_size) {
      return nullptr;
   }

   cat::int4 const binding = symbol.info >> 4;
   if (binding != 1 && binding != 2 && binding != 10) {
      return nullptr;
   }

   unsigned char const type = symbol.info & 0xf;
   if (type != 0 && type != 1 && type != 2 && type != 5) {
      return nullptr;
   }

   // The string table must hold `name` followed by its terminator.
   char const* const p_entry = table.p_strings + symbol.name;
   if (
      cat::idx(table.strings_size - symbol.name) <= name.size()
      || p_entry[name.size()] != '\0'
      || cat::str_view(p_entry, name.size()) != name
   ) {
      return nullptr;
   }

   return __builtin_bit_cast(void*, table.load_bias + symbol.value);
}

[[nodiscard]]
auto
count_symbols_gnu(cat::uint4 const* _Nonnull p_gnu_hash) -> cat::idx {
   // Unlike GlibC, I don't use a bloom filter, because I can't produce a
   // reasonable case where it's helpful and nobody else but GlibC seems to
   // use it either.

   // `DT_GNU_HASH` does not store a symbol count, so it's parsed out of from
   // the highest bucket.
   cat::uint4 const bucket_count = p_gnu_hash[0];
   cat::uint4 const symbol_offset = p_gnu_hash[1];
   cat::uint4 const bloom_size = p_gnu_hash[2];
   if (
      bucket_count == 0u || bucket_count > maximum_symbols || bloom_size == 0u
      || bloom_size > maximum_symbols
   ) {
      return 0u;
   }

   auto const* const p_bloom =
      __builtin_bit_cast(cat::uint8 const*, p_gnu_hash + 4);
   auto const* const p_buckets =
      __builtin_bit_cast(cat::uint4 const*, p_bloom + bloom_size);
   cat::uint4 const* const p_chains = p_buckets + bucket_count;

   cat::idx last_symbol = 0u;
   for (cat::idx index = 0u; index < bucket_count; ++index) {
      if (last_symbol < p_buckets[index]) {
         last_symbol = p_buckets[index];
      }
   }

   if (last_symbol < symbol_offset) {
      return symbol_offset;
   }

   while (last_symbol < maximum_symbols
          && (p_chains[cat::idx(last_symbol - symbol_offset)] & 1u) == 0u) {
      ++last_symbol;
   }

   return last_symbol + 1u;
}

// The vDSO's `"Linux"` note of type 0 holds the running kernel's
// `LINUX_VERSION_CODE`, packed as `major << 16 | minor << 8 | patch`.
[[nodiscard]]
auto
find_kernel_version(cat::uint1 const* _Nonnull p_notes, cat::idx notes_size)
   -> cat::maybe<nix::kernel_version> {
   constexpr cat::idx note_header_size = sizeof(elf_note);
   cat::idx offset = 0u;
   while (offset + note_header_size <= notes_size) {
      elf_note note;
      __builtin_memcpy_inline(&note, p_notes + offset, sizeof(note));

      // Names and descriptors are padded to 4 bytes.
      cat::idx const name_size = (note.name_size + 3u) & ~3u;
      cat::idx const descriptor_size = (note.descriptor_size + 3u) & ~3u;
      cat::idx const name_offset = offset + note_header_size;
      cat::idx const descriptor_offset = name_offset + name_size;
      cat::idx const next_offset = descriptor_offset + descriptor_size;

      if (next_offset > notes_size) {
         return cat::nullopt;
      }

      if (
         note.type == 0u && note.name_size == 6u && note.descriptor_size == 4u
         && cat::str_view(
               __builtin_bit_cast(char const*, p_notes + name_offset), 5u
            ) == "Linux"
      ) {
         cat::uint4 code;
         __builtin_memcpy_inline(&code, p_notes + descriptor_offset, 4u);
         // The shifts and mask make these fit.
         return nix::kernel_version{
            cat::narrow_cast<cat::int4>(code >> 16u).value(),
            cat::narrow_cast<cat::int4>((code >> 8u) & 0xffu).value(),
         };
      }

      offset = next_offset;
   }

   return cat::nullopt;
}

// Parse `.dynsym` for the symbol name.
[[nodiscard]]
auto
lookup_elf_symbol(symbol_table const& table, cat::str_view name)
   -> void* _Nullable {
   for (cat::idx index = 0u; index < table.symbol_count; ++index) {
      if (void* const p_result = symbol_address(table, index, name)) {
         return p_result;
      }
   }
   return nullptr;
}

}  // namespace

void
nix::detail::initialize_vdso(void const* _Nullable p_elf_header_value) {
   if (p_elf_header_value == nullptr) {
      return;
   }

   auto const* const p_elf_header =
      __builtin_bit_cast(elf_header const*, p_elf_header_value);
   if (
      p_elf_header->identification[0] != 0x7f
      || p_elf_header->identification[1] != 'E'
      || p_elf_header->identification[2] != 'L'
      || p_elf_header->identification[3] != 'F'
      || p_elf_header->identification[4] != 2
      || p_elf_header->identification[5] != 1
      || p_elf_header->identification[6] != 1
      || p_elf_header->program_header_size != sizeof(elf_program_header)
      || p_elf_header->program_header_count == 0u
      || p_elf_header->program_header_count > maximum_program_headers
   ) {
      return;
   }

   cat::uint8 const header_address =
      __builtin_bit_cast(cat::uint8, p_elf_header_value);
   auto const* const p_program_headers = __builtin_bit_cast(
      elf_program_header const*,
      header_address + p_elf_header->program_header_offset
   );

   cat::uint8 load_bias = 0u;
   for (cat::idx index = 0u; index < p_elf_header->program_header_count;
        ++index) {
      elf_program_header const& header = p_program_headers[index];

      if (header.type == 1u) {
         load_bias = header_address + header.offset - header.virtual_address;
         break;
      }
   }

   if (load_bias == 0u) {
      return;
   }

   for (cat::idx index = 0u; index < p_elf_header->program_header_count;
        ++index) {
      elf_program_header const& header = p_program_headers[index];
      if (header.type == 4u) {
         nix::detail::vdso_kernel_version = find_kernel_version(
            __builtin_bit_cast(
               cat::uint1 const*, load_bias + header.virtual_address
            ),
            cat::narrow_cast<cat::idx>(header.file_size).assert()
         );

         break;
      }
   }

   elf_dynamic const* p_dynamic = nullptr;
   cat::idx dynamic_count = 0u;

   for (cat::idx index = 0u; index < p_elf_header->program_header_count;
        ++index) {
      elf_program_header const& header = p_program_headers[index];

      if (header.type == 2u) {
         p_dynamic = __builtin_bit_cast(
            elf_dynamic const*, load_bias + header.virtual_address
         );

         dynamic_count = cat::idx(header.file_size / sizeof(elf_dynamic));
         if (dynamic_count > maximum_dynamic_entries) {
            dynamic_count = maximum_dynamic_entries;
         }

         break;
      }
   }

   if (p_dynamic == nullptr || dynamic_count == 0u) {
      return;
   }

   symbol_table table{.load_bias = load_bias};
   cat::uint4 const* p_hash = nullptr;
   cat::uint4 const* p_gnu_hash = nullptr;
   bool is_dynamic_terminated = false;
   for (cat::idx index = 0u; index < dynamic_count; ++index) {
      elf_dynamic const& dynamic = p_dynamic[index];
      if (dynamic.tag == 0) {
         is_dynamic_terminated = true;
         break;
      }

      switch (dynamic.tag.raw) {
         case 4:
            p_hash =
               __builtin_bit_cast(cat::uint4 const*, load_bias + dynamic.value);
            break;
         case 5:
            table.p_strings =
               __builtin_bit_cast(char const*, load_bias + dynamic.value);
            break;
         case 6:
            table.p_symbols =
               __builtin_bit_cast(elf_symbol const*, load_bias + dynamic.value);
            break;
         case 10:
            table.strings_size = dynamic.value;
            break;
         case 0x6ffffef5:
            p_gnu_hash =
               __builtin_bit_cast(cat::uint4 const*, load_bias + dynamic.value);
            break;
         default:
            // The vDSO carries many other tags that we do not read.
            break;
      }
   }

   if (
      !is_dynamic_terminated || table.p_strings == nullptr
      || table.p_symbols == nullptr || table.strings_size == 0u
   ) {
      return;
   }

   // The second word of `DT_HASH` is its chain length, which is the number of
   // entries in `.dynsym`. arm64 ships the vDSO without `DT_HASH`, so there
   // the count comes out of `DT_GNU_HASH` instead.
   if (p_hash != nullptr) {
      table.symbol_count = p_hash[1];
   } else if (p_gnu_hash != nullptr) {
      table.symbol_count = count_symbols_gnu(p_gnu_hash);
   }

   if (table.symbol_count == 0u) {
      return;
   }

   if (table.symbol_count > maximum_symbols) {
      table.symbol_count = maximum_symbols;
   }

   vdso_symbols = table;
}

auto
nix::detail::lookup_vdso_symbol(
   cat::str_view vdso_name, cat::str_view alias
) noexcept -> void* _Nullable {
   void* p_symbol = lookup_elf_symbol(vdso_symbols, vdso_name);

   if (p_symbol == nullptr) {
      p_symbol = lookup_elf_symbol(vdso_symbols, alias);
   }

   return p_symbol;
}
