#include <cat/array>
#include <cat/defer>
#include <cat/file_path>
#include <cat/inplace_allocator>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto
main() -> int {
   // This leading \0 denotes the abstract namespace.
   constexpr cat::zstr_view socket_name = "\0/tmp/temp.sock";

   // TODO: We should have a `file_path_inplace`.
   auto allocator = cat::make_inplace_allocator<64u>();
   cat::file_path socket_path =
      cat::make_file_path(allocator, socket_name).verify();
   $defer {
      socket_path.free(allocator);
   };

   cat::socket_handle listening_socket =
      cat::socket_handle::create(
         cat::socket_domain::local, cat::socket_type::stream
      )
         .verify();
   listening_socket.bind(socket_path).verify();
   listening_socket.listen(20).verify();

   cat::socket_handle receiving_socket;
   // A fixed-size string's `.size()` is its whole capacity, so this offers
   // every byte of the buffer to `.receive()`.
   cat::str_inplace_fixed<108> message_buffer;

   bool exit = false;
   while (!exit) {
      receiving_socket = listening_socket.accept4().verify();

      while (true) {
         cat::idx const received =
            receiving_socket
               .receive(
                  cat::span<char>(message_buffer.data(), message_buffer.size())
               )
               .verify();

         // A zero-length read means that this client hung up.
         if (received == 0u) {
            break;
         }

         // Only the bytes that just arrived are part of the message.
         cat::str_view const input(message_buffer.data(), received);

         if (input == "exit") {
            cat::println("Closing the server.").or_exit();
            exit = true;
            break;
         }

         auto _ = cat::print("Recieved: ");
         cat::println(input).or_exit();
      }
   }

   receiving_socket.close().verify();
   listening_socket.close().verify();
}
