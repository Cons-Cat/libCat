#include <cat/array>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto
main(int argc, char* p_argv[]) -> int {
   cat::socket_unix<cat::socket_type::stream> socket;
   socket.path_name = cat::make_str_inplace<108>("\0/tmp/temp.sock");

   socket.create().verify();
   socket.connect().verify();

   // Send all command line arguments to the server.
   for (int i = 1; i < argc; ++i) {
      socket.send_string(p_argv[i]).verify();
      if (i < argc - 1) {
         socket.send_string(" ").verify();
      }
   }

   socket.close().verify();
}
