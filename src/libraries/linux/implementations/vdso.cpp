#include <cat/detail/vdso.hpp>

#include <cat/array>

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

struct elf_dynamic {
   cat::int8 tag;
   cat::uint8 value;
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
strings_equal(
   char const* _Nonnull p_first, cat::idx first_size,
   char const* _Nonnull p_second
) -> bool {
   for (cat::idx index = 0u; index < first_size; ++index) {
      if (p_first[index] != p_second[index]) {
         return false;
      }
      if (p_first[index] == '\0') {
         return true;
      }
   }
   return false;
}

[[nodiscard]]
auto
symbol_address(
   symbol_table const& table, cat::idx index, char const* _Nonnull p_name
) -> void* _Nullable {
   if (index >= table.symbol_count) {
      return nullptr;
   }
   elf_symbol const& symbol = table.p_symbols[index];
   if (symbol.section_index == 0u || symbol.name >= table.strings_size) {
      return nullptr;
   }
   // A scan reaches local symbols that a hash probe never would, so filter on
   // the binding: global, weak, or unique.
   cat::int4 const binding = symbol.info >> 4;
   if (binding != 1 && binding != 2 && binding != 10) {
      return nullptr;
   }
   unsigned char const type = symbol.info & 0xf;
   if (type != 0 && type != 1 && type != 2 && type != 5) {
      return nullptr;
   }
   if (!strings_equal(
          table.p_strings + symbol.name,
          cat::idx(table.strings_size - symbol.name), p_name
       )) {
      return nullptr;
   }
   return __builtin_bit_cast(void*, table.load_bias + symbol.value);
}

// `DT_GNU_HASH` does not store a symbol count, so recover it from the highest
// bucket and then walk that bucket's chain to its terminator. Unlike GlibC,
// we don't use a bloom filter, because it the performance to size tradeoff
// seems unfavorable and ONLY GlibC does that.
[[nodiscard]]
auto
count_symbols_gnu(cat::uint4 const* _Nonnull p_gnu_hash) -> cat::idx {
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

// Scan `.dynsym` for the symbol name.
[[nodiscard]]
auto
lookup(symbol_table const& table, char const* _Nonnull p_name)
   -> void* _Nullable {
   for (cat::idx index = 0u; index < table.symbol_count; ++index) {
      if (void* const p_result = symbol_address(table, index, p_name)) {
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
   bool dynamic_terminated = false;
   for (cat::idx index = 0u; index < dynamic_count; ++index) {
      elf_dynamic const& dynamic = p_dynamic[index];
      if (dynamic.tag == 0) {
         dynamic_terminated = true;
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
      !dynamic_terminated || table.p_strings == nullptr
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
   char const* _Nonnull p_vdso_name, char const* _Nonnull p_alias
) -> void* _Nullable {
   void* p_symbol = lookup(vdso_symbols, p_vdso_name);
   if (p_symbol == nullptr) {
      p_symbol = lookup(vdso_symbols, p_alias);
   }
   return p_symbol;
}
