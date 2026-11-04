#include <cat/defer>
#include <cat/linux>

namespace {

constexpr cat::idx max_region_kb = 1'024u * 1'024u;
constexpr cat::idx max_anon_region_kb = 65'536u;

constexpr cat::array<cat::str_view, 5> skip_anon_prefixes{
   "[stack", "[heap]", "[vdso]", "[vvar]", "[vsyscall]",
};

struct smaps_totals {
   cat::idx resident_kb = 0u;
   cat::idx mapped_kb = 0u;
};

struct smaps_region {
   bool is_region = false;
   bool is_file_backed = false;
   bool is_private_anonymous = false;
   cat::idx size_kb = 0u;
   cat::idx rss_kb = 0u;  // Resident Set Size
};

auto
has_prefix(cat::str_view string, cat::str_view prefix) -> bool {
   return string.size() >= prefix.size()
          && string.substring(0u, prefix.size()) == prefix;
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
      }
   }
   return {};
}

auto
parse_kb_value(cat::str_view line) -> cat::idx {
   cat::maybe const colon = line.find(':');
   if (colon.is_empty()) {
      return 0u;
   }
   cat::idx i = colon.value() + 1u;
   while (i < line.size() && line[i] == ' ') {
      ++i;
   }
   cat::idx value = 0u;
   while (i < line.size() && line[i] >= '0' && line[i] <= '9') {
      value = cat::idx(value * 10u + line[i] - '0');
      ++i;
   }
   return value;
}

auto
is_region_header(cat::str_view line) -> bool {
   return line.size() != 0u && line[0] != ' ' && line[0] != '\t'
          && line.find('-').has_value();
}

auto
read_region_header(cat::str_view header) -> smaps_region {
   cat::str_view const permissions = smaps_field(header, 1u);
   cat::str_view const pathname = smaps_field(header, 5u);
   bool const is_file_backed = pathname.size() != 0u && pathname[0] == '/';
   bool is_skipped = false;
   for (cat::str_view prefix : skip_anon_prefixes) {
      is_skipped = is_skipped || has_prefix(pathname, prefix);
   }
   return {
      .is_region = true,
      .is_file_backed = is_file_backed,
      .is_private_anonymous = permissions.find('p').has_value()
                              && permissions.find('w').has_value()
                              && !is_file_backed && !is_skipped,
   };
}

void
flush_region(smaps_region const& region, smaps_totals& totals) {
   if (
      !region.is_region || region.is_file_backed
      || region.size_kb > max_anon_region_kb || region.size_kb > max_region_kb
   ) {
      return;
   }
   if (region.is_private_anonymous) {
      totals.resident_kb += region.rss_kb;
   }
   totals.mapped_kb += region.size_kb;
}

void
process_smaps_line(
   cat::str_view line, smaps_region& region, smaps_totals& totals
) {
   if (is_region_header(line)) {
      flush_region(region, totals);
      region = read_region_header(line);
   } else if (has_prefix(line, "Size:")) {
      region.size_kb = parse_kb_value(line);
   } else if (has_prefix(line, "Rss:")) {
      region.rss_kb = parse_kb_value(line);
   }
}

}  // namespace

// Perform `nix::sys_open()` and `nix::sys_read()` on `/proc/self/smaps`, the
// detailed memory-map file. This can fail with `nix::linux_error` of:
//    `noent` when `/proc` path is not mounted,
//    `mfile`/`nfile` at the file descriptor limit,
//    `nomem`,
//    `intr` on signal interruption.
auto
nix::read_self_anon_smaps() -> cat::scaredy<nix::anon_smaps, nix::linux_error> {
   nix::file_descriptor const fd =
      $prop(nix::sys_open("/proc/self/smaps", nix::open_mode::read_only));
   $defer {
      auto _ = nix::sys_close(fd);
   };

   // Lines are parsed as they are read, so the file can be any size. A line
   // too long for this buffer is cut short, which keeps the fields at the start
   // of a region header.
   cat::array<char, 4'096u> buffer;
   cat::idx used = 0u;
   bool is_skipping_cut_line = false;
   smaps_region region;
   smaps_totals totals;

   while (true) {
      cat::idx const bytes = $prop(
         nix::sys_read(fd, buffer.data() + used, cat::idx(buffer.size() - used))
      );
      if (bytes == 0u) {
         break;
      }
      used += bytes;

      cat::idx line_start = 0u;
      for (cat::idx i = 0u; i < used; ++i) {
         if (buffer[i] == '\n') {
            if (!is_skipping_cut_line) {
               process_smaps_line(
                  {buffer.data() + line_start, cat::idx(i - line_start)},
                  region, totals
               );
            }
            is_skipping_cut_line = false;
            line_start = i + 1u;
         }
      }

      if (line_start == 0u && used == buffer.size()) {
         if (!is_skipping_cut_line) {
            process_smaps_line({buffer.data(), used}, region, totals);
         }
         is_skipping_cut_line = true;
         used = 0u;
      } else {
         used = cat::idx(used - line_start);
         cat::copy_memory_backward(
            buffer.data() + line_start, buffer.data(), used
         );
      }
   }
   if (used != 0u && !is_skipping_cut_line) {
      process_smaps_line({buffer.data(), used}, region, totals);
   }
   flush_region(region, totals);

   return nix::anon_smaps{
      .resident_bytes = totals.resident_kb * 1'024u,
      .mapped_bytes = totals.mapped_kb * 1'024u,
   };
}
