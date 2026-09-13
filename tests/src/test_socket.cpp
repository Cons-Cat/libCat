#include <cat/linux>
#include <cat/page_allocator>
#include <cat/socket>

#include "../unit_tests.hpp"

$test(socket_address_values) {
   cat::array<cat::uint1, 16u> const bytes(
      0x20u, 0x01u, 0x0du, 0xb8u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u
   );
   cat::ipv6_socket_address const address(bytes, 0x1234u, 0x10203040u, 7u);

   static_assert(sizeof(address) == 28u);
   cat::verify(address.domain() == cat::socket_domain::ipv6);
   cat::verify(address.port() == 0x1234u);
   cat::verify(address.flow_info() == 0x10203040u);
   cat::verify(address.scope_id() == 7u);
   cat::verify(address.address()[0u] == 0x20u);
   cat::verify(address.address()[1u] == 0x01u);
   cat::verify(address.address()[2u] == 0x0du);
   cat::verify(address.address()[3u] == 0xb8u);
   cat::verify(address.address()[15u] == 1u);
}

$test(socket_handle_lifecycle_and_nonblocking) {
   auto handle = cat::socket_handle::create(
                    cat::socket_domain::local, cat::socket_type::stream,
                    cat::socket_flags::nonblocking
   )
                    .verify();
   cat::verify(handle.is_open());
   cat::verify(handle.native_handle().is_socket());
   cat::verify(handle.native_handle().is_kernel_handle());
   cat::verify(handle.native_handle().is_readable());
   cat::verify(handle.native_handle().is_writable());
   cat::verify(handle.native_handle().is_nonblocking());

   cat::iword const handle_value =
      cat::detail::native_handle_access::value(handle.native_handle());
   cat::idx const flags =
      nix::sys_fcntl(
         handle_value == -1 ? nix::invalid_file_descriptor
                            : nix::file_descriptor(cat::uint4(handle_value)),
         nix::fcntl_command::get_status_flags
      )
         .verify();
   cat::verify((flags.raw & 0x800u) != 0u);

   cat::native_handle const native = handle.native_handle();
   cat::socket_handle moved(cat::move(handle));
   cat::verify(!handle.is_open());
   cat::verify(moved.native_handle() == native);

   cat::socket_handle assigned;
   assigned = cat::move(moved);
   cat::verify(!moved.is_open());
   cat::verify(assigned.native_handle() == native);

   cat::native_handle released = assigned.release();
   cat::verify(!assigned.is_open());
   cat::iword const released_value =
      cat::detail::native_handle_access::value(released);
   nix::sys_fcntl(
      released_value == -1 ? nix::invalid_file_descriptor
                           : nix::file_descriptor(cat::uint4(released_value)),
      nix::fcntl_command::get_fd_flags
   )
      .verify();

   auto adopted = cat::socket_handle::adopt(cat::move(released));
   cat::verify(!released.is_valid());
   adopted.close().verify();
   cat::verify(!adopted.is_open());
   adopted.close().verify();
}

$test(unix_socket_stream_round_trip) {
   auto socket_path =
      cat::make_file_path_unique(pager, "/tmp/libcat-socket-%%%%").verify();
   $defer {
      auto _ = nix::sys_unlink(socket_path.native());
      socket_path.free(pager);
   };
   auto _ = nix::sys_unlink(socket_path.native());

   cat::unix_socket_address const address(socket_path);
   cat::verify(!address.is_abstract());
   cat::verify(address.path_size() == socket_path.size());

   auto server = cat::socket_handle::create(
                    cat::socket_domain::local, cat::socket_type::stream
   )
                    .verify();
   server.bind(address).verify();
   server.listen(4).verify();

   auto client = cat::socket_handle::create(
                    cat::socket_domain::local, cat::socket_type::stream
   )
                    .verify();
   client.connect(socket_path).verify();
   cat::verify(client.send("ping").verify() == 4u);

   auto accepted = server.accept4(cat::socket_flags::nonblocking).verify();
   cat::iword const accepted_value =
      cat::detail::native_handle_access::value(accepted.native_handle());
   cat::idx const flags =
      nix::sys_fcntl(
         accepted_value == -1
            ? nix::invalid_file_descriptor
            : nix::file_descriptor(cat::uint4(accepted_value)),
         nix::fcntl_command::get_status_flags
      )
         .verify();
   cat::verify((flags.raw & 0x800u) != 0u);

   char request[4] = {};
   cat::verify(accepted.receive(request).verify() == 4u);
   cat::verify(request[0] == 'p');
   cat::verify(request[1] == 'i');
   cat::verify(request[2] == 'n');
   cat::verify(request[3] == 'g');

   cat::verify(accepted.send("ok").verify() == 2u);
   char response[2] = {};
   cat::verify(client.receive(response).verify() == 2u);
   cat::verify(response[0] == 'o');
   cat::verify(response[1] == 'k');

   accepted.close().verify();
   client.close().verify();
   server.close().verify();
   nix::sys_unlink(socket_path.native()).verify();
}

$test(ipv4_socket_port_zero) {
   constexpr cat::array<cat::uint1, 4u> loopback(127u, 0u, 0u, 1u);
   auto socket = cat::socket_handle::create(
                    cat::socket_domain::ipv4, cat::socket_type::stream
   )
                    .verify();
   socket.bind(cat::ipv4_socket_address(loopback, 0u)).verify();

   cat::ipv4_socket_address local;
   socket.getsockname(local).verify();
   cat::verify(local.domain() == cat::socket_domain::ipv4);
   cat::verify(local.address()[0u] == 127u);
   cat::verify(local.address()[1u] == 0u);
   cat::verify(local.address()[2u] == 0u);
   cat::verify(local.address()[3u] == 1u);
   cat::verify(local.port() != 0u);
}
