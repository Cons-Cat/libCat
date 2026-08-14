#include <cat/linux>

auto
nix::cpu_temperature() -> cat::maybe<cat::int4> {
   file_descriptor const descriptor = $prop_as(
      sys_open("/sys/class/thermal/thermal_zone0/temp", open_mode::read_only),
      cat::nullopt
   );
   $defer {
      // Closing this special descriptor can never fail.
      auto _ = sys_close(descriptor);
   };

   cat::array<char, 32> buffer;
   cat::idx length = 0u;
   while (length < buffer.size()) {
      scaredy_nix<cat::idx> read_result = sys_read(
         descriptor, buffer.data() + length,
         static_cast<cat::idx>(buffer.size() - length)
      );
      if (read_result.is_empty()) {
         if (read_result.error() == linux_error::intr) {
            continue;
         }
         return cat::nullopt;
      }

      cat::idx const count = read_result.value();
      if (count == 0) {
         break;
      }
      length += count;
   }

   cat::idx cursor = 0u;
   bool negative = false;
   if (cursor < length && (buffer[cursor] == '-' || buffer[cursor] == '+')) {
      negative = buffer[cursor] == '-';
      ++cursor;
   }

   cat::idx const first_digit = cursor;
   cat::uint4 magnitude = 0u;
   cat::uint4 const positive_limit = cat::int4_max;
   cat::uint4 const negative_limit = positive_limit + 1u;
   cat::uint4 const limit = negative ? negative_limit : positive_limit;
   // TODO: Something like `is_digit` and `to_digit` would be better.
   while (cursor < length && buffer[cursor] >= '0' && buffer[cursor] <= '9') {
      cat::uint4 const digit(buffer[cursor] - '0');
      if (magnitude > (limit - digit) / 10u) {
         return cat::nullopt;
      }
      magnitude = magnitude * 10u + digit;
      ++cursor;
   }
   if (cursor == first_digit) {
      return cat::nullopt;
   }

   while (
      cursor < length
      // TODO: Something like `is_whitespace` would be better.
      && (buffer[cursor] == '\n' || buffer[cursor] == '\r' || buffer[cursor] == ' ' || buffer[cursor] == '\t')) {
      ++cursor;
   }
   if (cursor != length) {
      return cat::nullopt;
   }

   if (negative) {
      if (magnitude == negative_limit) {
         return cat::int4_min;
      }
      return -cat::int4(magnitude);
   }
   return cat::int4(magnitude);
}
