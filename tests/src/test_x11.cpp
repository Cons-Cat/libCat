#include <cat/x11>

#include "../unit_tests.hpp"

// NOLINTBEGIN(readability-magic-numbers)

// A reply's `scaredy_x11` is exactly the wire packet, sharing storage with the
// error packet.
static_assert(sizeof(x11::scaredy_x11<x11::intern_atom_reply>) == 32);
static_assert(sizeof(x11::scaredy_x11<x11::geometry_reply>) == 32);
static_assert(sizeof(x11::scaredy_x11<void>) == 32);

$test(x11_scaredy) {
   x11::scaredy_x11<x11::intern_atom_reply> reply =
      x11::intern_atom_reply{.response_type = 1u, .atom = x11::atom(42u)};
   cat::verify(reply.has_value());
   cat::verify(reply.value().atom == x11::atom(42u));

   x11::error_packet bad_atom{};
   bad_atom.code = x11::error_code::atom;
   bad_atom.bad_value = 7u;
   reply = bad_atom;
   cat::verify(!reply.has_value());
   cat::verify(reply.error().code == x11::error_code::atom);
   cat::verify(reply.error().bad_value == 7u);

   x11::scaredy_x11<void> transport =
      x11::error_packet::from_linux(nix::linux_error::connreset);
   cat::verify(transport.error().is_transport());
   cat::verify(transport.error().linux_error() == nix::linux_error::connreset);
   x11::scaredy_x11<void> const success = cat::monostate;
   cat::verify(success.has_value());
}

$test(x11_display_name) {
   x11::display_name name = x11::parse_display_name(":0").verify();
   cat::verify(name.host.size() == 0u);
   cat::verify(name.display == 0u && name.screen == 0u);

   name = x11::parse_display_name("unix:12.3").verify();
   cat::verify(name.host == cat::str_view("unix"));
   cat::verify(name.display == 12u && name.screen == 3u);

   name = x11::parse_display_name("example.com:1").verify();
   cat::verify(name.host == cat::str_view("example.com"));

   cat::verify(!x11::parse_display_name("").has_value());
   cat::verify(!x11::parse_display_name("0").has_value());
   cat::verify(!x11::parse_display_name(":").has_value());
   cat::verify(!x11::parse_display_name(":x").has_value());
   cat::verify(!x11::parse_display_name(":1.").has_value());
}

$test(x11_authorization) {
   // Three Xauthority entries: another host, another display, and a match.
   cat::array<unsigned char, 102u> const file = {
      // Family local, host "other", display "7".
      1,
      0,
      0,
      5,
      'o',
      't',
      'h',
      'e',
      'r',
      0,
      1,
      '7',
      0,
      18,
      'M',
      'I',
      'T',
      '-',
      'M',
      'A',
      'G',
      'I',
      'C',
      '-',
      'C',
      'O',
      'O',
      'K',
      'I',
      'E',
      '-',
      '1',
      0,
      1,
      'a',
      // Family local, host "cat", display "3".
      1,
      0,
      0,
      3,
      'c',
      'a',
      't',
      0,
      1,
      '3',
      0,
      18,
      'M',
      'I',
      'T',
      '-',
      'M',
      'A',
      'G',
      'I',
      'C',
      '-',
      'C',
      'O',
      'O',
      'K',
      'I',
      'E',
      '-',
      '1',
      0,
      1,
      'b',
      // Family local, host "cat", display "7".
      1,
      0,
      0,
      3,
      'c',
      'a',
      't',
      0,
      1,
      '7',
      0,
      18,
      'M',
      'I',
      'T',
      '-',
      'M',
      'A',
      'G',
      'I',
      'C',
      '-',
      'C',
      'O',
      'O',
      'K',
      'I',
      'E',
      '-',
      '1',
      0,
      2,
      'c',
      'd',
   };
   cat::span<cat::byte const> const bytes(
      __builtin_bit_cast(cat::byte const*, file.data()), file.size()
   );

   x11::authorization const found =
      x11::find_authorization(bytes, "cat", 7u).verify();
   cat::verify(found.name == cat::str_view("MIT-MAGIC-COOKIE-1"));
   cat::verify(found.data == cat::str_view("cd"));

   cat::verify(!x11::find_authorization(bytes, "dog", 7u).has_value());
   cat::verify(!x11::find_authorization(bytes, "cat", 9u).has_value());
   // A truncated file finds nothing past the truncation.
   cat::verify(!x11::find_authorization(
                   cat::span<cat::byte const>(bytes.data(), 70u), "cat", 7u
   )
                   .has_value());
}

// NOLINTEND(readability-magic-numbers)
