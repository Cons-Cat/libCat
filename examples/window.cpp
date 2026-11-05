#include <cat/allocator_page>
#include <cat/raii_io_uring>
#include <cat/raii_x11>
#include <cat/string>

using cat::operator""_uki;

// Open two windows, exercise replies and errors, then close each window
// through the same close request a window manager sends.
auto
main(int argc, char* const* pp_argv) -> int {
   cat::allocator_page pager;

   // `x11::raii::connection` submits its socket operations to this ring.
   // `single_issuer` and `defer_taskrun` run completions only inside this
   // thread's `io_uring_enter`, without internal locking. `taskrun_flag`
   // reports when there are any to run, so callers can skip entering.
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::single_issuer
                  | nix::io_uring_setup_flags::defer_taskrun
                  | nix::io_uring_setup_flags::taskrun_flag
                  | nix::io_uring_setup_flags::submit_all
                  | nix::io_uring_setup_flags::no_sq_array;

   nix::raii::io_uring ring =
      nix::raii::make_io_uring(cat::allocator_dyn(pager), 64u, params).verify();
   ring.register_file_table(64u).verify();

   // TODO: We should streamline away the environment parameter.
   x11::raii::connection display =
      x11::raii::connection::open(
         cat::allocator_dyn(pager), ring, pp_argv + argc + 1
      )
         .verify();
   x11::screen const& screen = display.default_screen();

   cat::array<x11::window, 2u> windows;
   cat::array<bool, 2u> is_open = {true, true};

   for (x11::window& window : windows) {
      window = display.create_window({
         .width = 320u,
         .height = 240u,
         .attributes = {
                        .background_pixel = screen.black_pixel,
                        .event_mask =
               x11::event_mask::structure_notify | x11::event_mask::exposure,
                        },
      });

      display.set_title(window, "libCat");
      display.enable_close_request(window);
      display.map_window(window);
   }

   // A request on a window that does not exist fails with `BadWindow`.
   auto bad = display.check(display.map_window(x11::window(0xc0decafe)));
   cat::verify(bad.is_empty() && bad.error().code == x11::error_code::window);

   x11::geometry_reply const geometry =
      display.wait(display.get_geometry(windows[0])).verify();
   cat::verify(geometry.width == 320u);

   // Cookies can be waited in any order, but only once.
   auto const discarded = display.get_input_focus();
   display.discard(discarded);
   auto const protocols = display.intern_atom("WM_PROTOCOLS");
   auto const delete_window = display.intern_atom("WM_DELETE_WINDOW");
   x11::atom const delete_window_atom =
      display.wait(delete_window).verify().atom;
   x11::atom const protocols_atom = display.wait(protocols).verify().atom;
   auto const again = display.wait(protocols);
   auto const latest_again = display.wait(delete_window);
   auto const gone = display.wait(discarded);

   cat::verify(
      protocols_atom != x11::atom::none && delete_window_atom != x11::atom::none
      && again.is_empty()
      && again.error().linux_error == nix::linux_error::noent
      && latest_again.is_empty()
      && latest_again.error().linux_error == nix::linux_error::noent
      && gone.is_empty() && gone.error().linux_error == nix::linux_error::noent
   );

   // A request with a reply that fails keeps its error for its own wait, even
   // when a later request is waited first.
   auto const bad_geometry = display.get_geometry(x11::window(0xc0decafe));
   display.wait(display.get_input_focus()).verify();

   auto const geometry_error = display.wait(bad_geometry);
   cat::verify(
      geometry_error.is_empty()
      && geometry_error.error().code == x11::error_code::drawable
   );

   // A discarded request's error is dropped, rather than becoming an event.
   display.discard(display.get_geometry(x11::window(0xc0decafe)));

   // A request larger than the server accepts is not sent, which does not fail
   // the connection.
   static cat::array<char, 512_uki> huge{};
   auto const oversized = display.change_property(
      windows[0], x11::atoms::wm_name, x11::atoms::string,
      x11::property_format::bits_8,
      cat::as_bytes(cat::span(huge.data(), huge.size()))
   );

   auto const oversized_error = display.check(oversized);
   cat::verify(
      oversized.sequence == 0u && oversized_error.is_empty()
      && oversized_error.error().code == x11::error_code::length
   );

   // Lengths which do not fit their fields are rejected rather than
   // truncated.
   auto const long_name =
      display.wait(display.intern_atom(cat::str_view(huge.data(), 70'000u)));

   auto const partial_item = display.check(display.change_property(
      windows[0], x11::atoms::wm_name, x11::atoms::cardinal,
      x11::property_format::bits_32, cat::as_bytes(cat::span(huge.data(), 6u))
   ));

   cat::verify(
      long_name.is_empty() && long_name.error().code == x11::error_code::length
      && partial_item.is_empty()
      && partial_item.error().code == x11::error_code::length
   );

   display.wait(display.get_input_focus()).verify();

   // A request can be checked after waiting on later requests.
   auto const late = display.map_window(x11::window(0xc0decafe));
   display.wait(display.get_input_focus()).verify();
   auto const late_error = display.check(late);

   cat::verify(
      late_error.is_empty()
      && late_error.error().code == x11::error_code::window
   );

   // An unchecked request's error arrives as an event, matched by its cookie.
   auto const unchecked = display.map_window(x11::window(0xc0decafe));
   bool has_unchecked_error = false;

   cat::idx mapped = 0u;

   while (is_open[0] || is_open[1]) {
      x11::event const& event = *display.wait_event().verify();
      if (event.is_error()) {
         cat::verify(unchecked.is_for(event.error()));

         has_unchecked_error = true;
         continue;
      }

      cat::idx const index = event.window() == windows[0] ? 0u : 1u;

      // TODO: Rather than instantly closing, we should poll on user input.
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

   cat::verify(has_unchecked_error);
   display.flush().verify();
}
