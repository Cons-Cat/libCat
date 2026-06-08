#include <cat/linux>

auto
nix::read_self_statm() -> cat::scaredy<nix::self_statm, nix::linux_error> {
   nix::file_descriptor fd =
      $prop(nix::sys_open("/proc/self/statm", nix::open_mode::read_only));

   // `/proc/self/statm` is one short ASCII line. 128 bytes covers every
   // feasible output.
   cat::zstr_inplace<128> buffer = {};
   cat::scaredy read_result =
      nix::sys_read(fd, buffer.data(), buffer.size() - 1u);
   // Close the fd before propagating a read failure so the descriptor never
   // leaks across an early return.
   auto _ = nix::sys_close(fd);
   cat::iword length = $prop(read_result);

   cat::idx cursor = 0u;

   // TODO: Deduplicate this code throughout libCat.
   auto parse_decimal = [&] -> cat::uword {
      cat::uword value = 0u;
      while (cursor < length && buffer[cursor] >= '0'
             && buffer[cursor] <= '9') {
         value = value * 10u + buffer[cursor] - '0';
         ++cursor;
      }
      return value;
   };

   // The file has seven space-separated integers followed by a newline.
   // Only the first six are read since the trailing dirty-pages field has been
   // unused since Linux 2.6. `skip_spaces` tolerates a stray multi-space
   // separator without causing a parse failure.
   // TODO: We should use views instead of this manual work.
   auto skip_spaces = [&] {
      while (cursor < length && buffer[cursor] == ' ') {
         ++cursor;
      }
   };

   nix::self_statm result;
   cat::array<cat::uword*, 6> statm_fields{
      &result.total_pages,
      &result.resident_pages,
      &result.shared_pages,
      &result.text_pages,
      &result._,
      &result.data_pages,
   };

   for (auto&& field : statm_fields) {
      cat::assert(cursor < length);
      *field = parse_decimal();
      skip_spaces();
   }
   return result;
}
