#include <cat/allocator_inplace>
#include <cat/allocator_page>
#include <cat/raii_io_uring>
#include <cat/raii_x11>

// 64 entries take a page of SQEs and a page of rings.
alignas(
   cat::page_size.raw
) cat::allocator_inplace<cat::page_size * 2u> allocator;

auto
main(int argc, char* const* pp_argv) -> int {
   cat::allocator_page pager;

   // `x11::raii::connection` submits its socket operations to this ring.
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::single_issuer
                  | nix::io_uring_setup_flags::defer_taskrun
                  | nix::io_uring_setup_flags::taskrun_flag
                  | nix::io_uring_setup_flags::submit_all
                  | nix::io_uring_setup_flags::no_sq_array;

   nix::raii::io_uring ring =
      nix::raii::make_io_uring(cat::allocator_dyn(allocator), 64u, params)
         .verify();
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

   x11::atom const protocols_atom =
      display.wait(display.intern_atom("WM_PROTOCOLS")).verify().atom;
   x11::atom const delete_window_atom =
      display.wait(display.intern_atom("WM_DELETE_WINDOW")).verify().atom;

   cat::idx mapped = 0u;

   while (is_open[0] || is_open[1]) {
      x11::event const& event = *display.wait_event().verify();
      cat::idx const index = event.window() == windows[0] ? 0u : 1u;

      // TODO: Rather than instantly closing, we should poll on user input.
      if (event.code() == x11::event_code::map_notify && ++mapped == 2u) {
         // Request that both windows close.
         for (x11::window window : windows) {
            x11::client_message_event message{};
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

   display.flush().verify();
}
