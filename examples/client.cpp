#include <cat/array>
#include <cat/defer>
#include <cat/file_path>
#include <cat/inplace_allocator>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto
main(int argc, char* _Nonnull p_argv[]) -> int {
   // This leading \0 denotes the abstract namespace.
   constexpr cat::zstr_view socket_name = "\0/tmp/temp.sock";

   // TODO: We should have a `file_path_inplace`.
   auto allocator = cat::make_inplace_allocator<64u>();
   cat::file_path socket_path =
      cat::make_file_path(allocator, socket_name).verify();
   $defer {
      socket_path.free(allocator);
   };

   cat::socket_handle socket =
      cat::socket_handle::create(
         cat::socket_domain::local, cat::socket_type::stream
      )
         .verify();
   socket.connect(socket_path)
      .or_exit("Failed to connect! Is the server running?");

   // Send all command line arguments to the server.
   for (int i = 1; i < argc; ++i) {
      socket.send(p_argv[i]).verify();
      if (i < argc - 1) {
         socket.send(" ").verify();
      }
   }

   socket.close().verify();
}
