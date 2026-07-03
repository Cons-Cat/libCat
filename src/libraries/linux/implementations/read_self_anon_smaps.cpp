#include <cat/linux>

namespace {

constexpr cat::idx max_region_kb = 1'024u * 1'024u;
constexpr cat::idx max_anon_region_kb = 65'536u;
constexpr cat::idx smaps_buffer_bytes = 256u * 1'024u;

constexpr cat::array<cat::str_view, 5> skip_anon_prefixes{
   "[stack", "[heap]", "[vdso]", "[vvar]", "[vsyscall]",
};

struct smaps_totals {
   cat::idx resident_kb = 0u;
   cat::idx total_mapped_kb = 0u;
   cat::idx file_mapped_kb = 0u;
};

struct smaps_region {
   cat::str_view header;
   cat::idx size_kb = 0u;
   cat::idx rss_kb = 0u; // Resident Set Size
};

auto
tag_has_prefix(cat::str_view tag, cat::str_view prefix) -> bool {
   if (tag.size() < prefix.size()) {
      return false;
   }
   return tag.substring(0u, prefix.size()) == prefix;
}

auto
skip_anon_tag(cat::str_view tag) -> bool {
   for (auto&& prefix : skip_anon_prefixes) {
      if (tag_has_prefix(tag, prefix)) {
         return true;
      }
   }
   return false;
}

auto
smaps_field(cat::str_view header, cat::idx index) -> cat::str_view {
   cat::idx field = 0u;
   cat::idx start = 0u;
   for (cat::idx i = 0u; i <= header.size(); ++i) {
      if (i == header.size() || header[i] == ' ' || header[i] == '\t') {
         if (i > start) {
            if (field == index) {
               return header.substring(start, cat::idx(i - start));
            }
            ++field;
         }
         while (i < header.size() && (header[i] == ' ' || header[i] == '\t')) {
            ++i;
         }
         start = i;
         continue;
      }
   }
   return {};
}

auto
parse_hex_uword(cat::str_view digits) -> cat::uword {
   cat::uword value = 0u;
   for (cat::idx i = 0u; i < digits.size(); ++i) {
      char const digit_char = digits[i];
      cat::uword digit = 0u;
      if (digit_char >= '0' && digit_char <= '9') {
         digit = cat::uword(digit_char - '0');
      } else if (digit_char >= 'a' && digit_char <= 'f') {
         digit = cat::uword(digit_char - 'a' + 10);
      } else if (digit_char >= 'A' && digit_char <= 'F') {
         digit = cat::uword(digit_char - 'A' + 10);
      } else {
         break;
      }
      value = value * 16u + digit;
   }
   return value;
}

struct address_span {
   cat::uword low;
   cat::uword high;
};

auto
region_addresses(cat::str_view header) -> address_span {
   cat::maybe const dash = header.find('-');
   if (!dash.has_value()) {
      return {.low = 0u, .high = 0u};
   }
   cat::uword const lo = parse_hex_uword(header.substring(0u, dash.value()));
   cat::str_view after_dash = header.remove_prefix(dash.value() + 1u);
   cat::maybe const space = after_dash.find(' ');
   cat::idx const hi_len =
      space.has_value() ? space.value() : after_dash.size();
   cat::uword const hi = parse_hex_uword(after_dash.substring(0u, hi_len));
   return {.low = lo, .high = hi};
}

auto
smaps_pathname(cat::str_view header) -> cat::str_view {
   return smaps_field(header, 5u);
}

auto
is_file_backed(cat::str_view header) -> bool {
   cat::str_view const tag = smaps_pathname(header);
   return tag.size() != 0u && tag[0] == '/';
}

auto
is_private_anonymous_map(cat::str_view header) -> bool {
   if (header.size() < 5u) {
      return false;
   }
   cat::str_view const perms = smaps_field(header, 1u);
   bool has_private = false;
   bool has_write = false;
   for (cat::idx i = 0u; i < perms.size(); ++i) {
      if (perms[i] == 'p') {
         has_private = true;
      }
      if (perms[i] == 'w') {
         has_write = true;
      }
   }
   if (!has_private || !has_write) {
      return false;
   }
   cat::str_view const tag = smaps_pathname(header);
   if (tag.size() != 0u) {
      if (tag[0] == '/') {
         return false;
      }
      if (skip_anon_tag(tag)) {
         return false;
      }
   }
   address_span const addresses = region_addresses(header);
   return addresses.high > addresses.low;
}

auto
region_span_valid(cat::str_view header, cat::idx size_kb) -> bool {
   address_span const addresses = region_addresses(header);
   if (addresses.high <= addresses.low) {
      return false;
   }
   if (size_kb > max_region_kb) {
      return false;
   }
   if (size_kb > max_anon_region_kb && !is_file_backed(header)) {
      cat::str_view const tag = smaps_pathname(header);
      if (skip_anon_tag(tag)) {
         return false;
      }
      return false;
   }
   return true;
}

void
flush_region(smaps_region const& region, smaps_totals& totals) {
   if (region.header.size() == 0u) {
      return;
   }
   if (!region_span_valid(region.header, region.size_kb)) {
      return;
   }
   if (is_private_anonymous_map(region.header)) {
      totals.resident_kb += region.rss_kb;
   }
   totals.total_mapped_kb += region.size_kb;
   if (is_file_backed(region.header)) {
      totals.file_mapped_kb += region.size_kb;
   }
}

auto
parse_kb_value(cat::str_view line) -> cat::maybe<cat::idx> {
   cat::maybe const colon = line.find(':');
   if (!colon.has_value()) {
      return cat::nullopt;
   }
   cat::str_view rest = line.remove_prefix(colon.value() + 1u);
   cat::idx i = 0u;
   while (i < rest.size() && rest[i] == ' ') {
      ++i;
   }
   if (i >= rest.size() || rest[i] < '0' || rest[i] > '9') {
      return cat::nullopt;
   }
   cat::idx value = 0u;
   while (i < rest.size() && rest[i] >= '0' && rest[i] <= '9') {
      value = cat::idx(value * 10u + rest[i] - '0');
      ++i;
   }
   return value;
}

auto
is_region_header(cat::str_view line) -> bool {
   if (line.size() == 0u || line[0] == ' ' || line[0] == '\t') {
      return false;
   }
   return line.find('-').has_value();
}

void
process_smaps_line(cat::str_view line, smaps_region& region,
                   smaps_totals& totals) {
   if (line.size() == 0u) {
      return;
   }
   if (is_region_header(line)) {
      flush_region(region, totals);
      region = {};
      region.header = line;
      return;
   }
   if (tag_has_prefix(line, "Size:")) {
      region.size_kb = parse_kb_value(line).value_or(0u);
   } else if (tag_has_prefix(line, "Rss:")) {
      region.rss_kb = parse_kb_value(line).value_or(0u);
   }
}

void
parse_smaps_buffer(cat::str_view buffer, smaps_totals& totals) {
   smaps_region region{};
   cat::idx line_start = 0u;
   for (cat::idx i = 0u; i <= buffer.size(); ++i) {
      if (i == buffer.size() || buffer[i] == '\n') {
         process_smaps_line(
            buffer.substring(line_start, cat::idx(i - line_start)), region,
            totals);
         line_start = i + 1u;
      }
   }
   flush_region(region, totals);
}

}  // namespace

// `open(2)`/`read(2)` on `/proc/self/smaps`, the detailed memory-map file, can
// fail with `noent` when `/proc` is not mounted, `mfile`/`nfile` at the fd
// limit, `nomem`, or `intr` on signal interruption.
auto
nix::read_self_anon_smaps()
   -> cat::scaredy<nix::anon_smaps, nix::linux_error> {
   nix::file_descriptor fd =
      $prop(nix::sys_open("/proc/self/smaps", nix::open_mode::read_only));

   cat::array<char, smaps_buffer_bytes> buffer = {};
   cat::idx total_bytes = 0u;

   while (true) {
      cat::scaredy read_result =
         nix::sys_read(fd, buffer.data() + total_bytes,
                       cat::iword(smaps_buffer_bytes.raw - total_bytes.raw));
      if (!read_result.has_value()) {
         auto _ = nix::sys_close(fd);
         return read_result.error();
      }
      cat::iword const nbytes = read_result.value();
      if (nbytes == 0) {
         break;
      }
      total_bytes += cat::idx(nbytes);
      if (total_bytes >= smaps_buffer_bytes) {
         auto _ = nix::sys_close(fd);
         return nix::linux_error::overflow;
      }
   }
   auto _ = nix::sys_close(fd);

   smaps_totals totals{};
   parse_smaps_buffer({buffer.data(), total_bytes}, totals);

   cat::idx const mapped_kb =
      cat::idx(totals.total_mapped_kb.raw - totals.file_mapped_kb.raw);
   return nix::anon_smaps{
      .resident_bytes = totals.resident_kb * 1'024u,
      .mapped_bytes = mapped_kb * 1'024u,
   };
}
