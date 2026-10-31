#include <cat/allocator_page>
#include <cat/string>
#include <cat/x11>

using cat::operator""_uki;

// Open two windows, exercise replies and errors, then close each window
// through the same close request a window manager sends.
auto
run(char const* _Nullable const* _Nonnull p_environment)
   -> x11::scaredy_x11<void> {
   cat::allocator_page pager;
   x11::connection display =
      $prop(x11::connection::open(cat::allocator_dyn(pager), p_environment));
   x11::screen const& screen = display.default_screen();

   cat::array<x11::window, 2u> windows;
   cat::array<bool, 2u> is_open = {true, true};
   for (x11::window& window : windows) {
      window = display.create_window({
         .width = 320u,
         .height = 240u,
         .attributes =
            x11::window_attributes{}
               .set(x11::window_attribute::background_pixel, screen.black_pixel)
               .set(
                  x11::window_attribute::event_mask,
                  cat::to_underlying(
                     x11::event_mask::structure_notify
                     | x11::event_mask::exposure
                  )
               ),
      });
      display.set_title(window, "libCat");
      display.enable_close_request(window);
      display.map_window(window);
   }

   // A request on a window that does not exist fails with `BadWindow`.
   auto bad = display.check(display.map_window(x11::window(0xdeadbeefu)));
   if (bad.has_value() || bad.error().code != x11::error_code::window) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }

   x11::geometry_reply const geometry =
      $prop(display.wait(display.get_geometry(windows[0])));
   if (geometry.width != 320u) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }

   // Cookies can be waited in any order, but only once.
   auto const discarded = display.get_input_focus();
   display.discard(discarded);
   auto const protocols = display.intern_atom("WM_PROTOCOLS");
   auto const delete_window = display.intern_atom("WM_DELETE_WINDOW");
   x11::atom const delete_window_atom = $prop(display.wait(delete_window)).atom;
   x11::atom const protocols_atom = $prop(display.wait(protocols)).atom;
   auto const again = display.wait(protocols);
   auto const latest_again = display.wait(delete_window);
   auto const gone = display.wait(discarded);
   if (
      protocols_atom == x11::atom::none || delete_window_atom == x11::atom::none
      || again.has_value()
      || again.error().linux_error() != nix::linux_error::noent
      || latest_again.has_value()
      || latest_again.error().linux_error() != nix::linux_error::noent
      || gone.has_value()
      || gone.error().linux_error() != nix::linux_error::noent
   ) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }

   // A request with a reply that fails keeps its error for its own wait, even
   // when a later request is waited first.
   auto const bad_geometry = display.get_geometry(x11::window(0xdeadbeefu));
   auto _ = $prop(display.wait(display.get_input_focus()));
   auto const geometry_error = display.wait(bad_geometry);
   if (
      geometry_error.has_value()
      || geometry_error.error().code != x11::error_code::drawable
   ) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }

   // A discarded request's error is dropped, rather than becoming an event.
   display.discard(display.get_geometry(x11::window(0xdeadbeefu)));

   // A request larger than the server accepts is not sent, which does not fail
   // the connection.
   static cat::array<cat::byte, 512_uki> huge{};
   auto const oversized = display.change_property(
      windows[0], x11::atoms::wm_name, x11::atoms::string,
      x11::property_format::bits_8,
      cat::span<cat::byte const>(huge.data(), huge.size())
   );
   auto const oversized_error = display.check(oversized);
   if (
      oversized.sequence != 0u || oversized_error.has_value()
      || oversized_error.error().code != x11::error_code::length
   ) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }
   // Lengths which do not fit their fields are rejected rather than
   // truncated.
   auto const long_name = display.wait(display.intern_atom(
      cat::str_view(__builtin_bit_cast(char const*, huge.data()), 70'000u)
   ));
   auto const partial_item = display.check(display.change_property(
      windows[0], x11::atoms::wm_name, x11::atoms::cardinal,
      x11::property_format::bits_32, cat::span<cat::byte const>(huge.data(), 6u)
   ));
   if (
      long_name.has_value() || long_name.error().code != x11::error_code::length
      || partial_item.has_value()
      || partial_item.error().code != x11::error_code::length
   ) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }
   auto _ = $prop(display.wait(display.get_input_focus()));

   // A request can be checked after waiting on later requests.
   auto const late = display.map_window(x11::window(0xdeadbeefu));
   auto _ = $prop(display.wait(display.get_input_focus()));
   auto const late_error = display.check(late);
   if (
      late_error.has_value()
      || late_error.error().code != x11::error_code::window
   ) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }

   // An unchecked request's error arrives as an event, matched by its cookie.
   auto const unchecked = display.map_window(x11::window(0xdeadbeefu));
   bool has_unchecked_error = false;

   cat::idx mapped = 0u;
   while (is_open[0] || is_open[1]) {
      x11::event const& event = *$prop(display.wait_event());
      if (event.is_error()) {
         if (!unchecked.is_for(event.as<x11::error_packet>())) {
            return event.as<x11::error_packet>();
         }
         has_unchecked_error = true;
         continue;
      }
      cat::idx const index = event.window() == windows[0] ? 0u : 1u;
      if (event.code() == x11::event_code::map_notify && ++mapped == 2u) {
         // Ask both windows to close, like a window manager would.
         for (x11::window window : windows) {
            x11::client_message_event message{};
            message.response_type =
               cat::to_underlying(x11::event_code::client_message);
            message.format = 32u;
            message.window = window;
            message.type = protocols_atom;
            message.data_32[0] = delete_window_atom.value;
            display.send_event(window, x11::event_mask::none, &message);
         }
      } else if (display.is_close_request(event)) {
         display.destroy_window(windows[index]);
         is_open[index] = false;
      }
   }
   if (!has_unchecked_error) {
      return x11::error_packet::from_linux(nix::linux_error::proto);
   }
   return display.flush();
}

auto
main(int argc, char* const* pp_argv) -> int {
   // `x11::connection` submits its socket operations to this thread's ring.
   if (nix::alloc_io_uring_runtime().is_empty()) {
      return 1;
   }
   int const status = run(pp_argv + argc + 1).has_value() ? 0 : 1;
   nix::free_io_uring_runtime();
   return status;
}
