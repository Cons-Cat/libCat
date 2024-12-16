#include <cat/array>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto
main() -> int {
   cat::socket_unix<cat::socket_type::stream> listening_socket;
   // A leading null byte puts this path in the abstract namespace.
   listening_socket.path_name = cat::make_str_inplace<108>("\0/tmp/temp.sock");
   listening_socket.create().verify();
   listening_socket.bind().verify();
   listening_socket.listen(20).verify();

   cat::socket_unix<cat::socket_type::stream> recieving_socket;
   cat::str_inplace<12> message_buffer;

   bool exit = false;
   while (!exit) {
      recieving_socket.accept(listening_socket).verify();

      while (true) {
         auto _ = recieving_socket
                     .recieve(message_buffer.data(), message_buffer.size())
                     .verify();

         cat::str_view const input = message_buffer.data();

         // TODO: This comparison is always false.
         if (cat::compare_strings(input, "exit")) {
            cat::println("Closing the server.").or_exit();
            exit = true;
            break;
         }

         auto _ = cat::print("Recieved: ");
         cat::println(input).or_exit();
         break;
      }
   }

   recieving_socket.close().verify();
   listening_socket.close().verify();
   nix::sys_unlink(listening_socket.path_name.data()).verify();
}
