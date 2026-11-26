#include <cat/array>
#include <cat/file_path_inplace>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto
main() -> int {
   cat::file_path_inplace<16u> const socket_path =
      // This leading \0 denotes the abstract namespace.
      cat::make_file_path_inplace<16u>("\0/tmp/temp.sock").verify();

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
}
