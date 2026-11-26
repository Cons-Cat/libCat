#include <cat/array>
#include <cat/file_path_inplace>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto
main(int argc, char* _Nonnull p_argv[]) -> int {
   cat::file_path_inplace<16u> const socket_path =
      // This leading \0 denotes the abstract namespace.
      cat::make_file_path_inplace<16u>("\0/tmp/temp.sock").verify();

   cat::socket_handle socket =
      cat::socket_handle::create(
         cat::socket_domain::local, cat::socket_type::stream
      )
         .verify();

   socket.connect(socket_path)
      .or_exit("Failed to connect! Is the server running?");

   // Send all command line arguments to the server.
   for (cat::idx i = 1; i < argc; ++i) {
      socket.send(p_argv[i]).verify();
      if (i < argc - 1) {
         socket.send(" ").verify();
      }
   }
}
