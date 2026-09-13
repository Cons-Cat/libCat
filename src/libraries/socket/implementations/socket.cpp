#include <cat/linux>
#include <cat/socket>

namespace cat {

namespace {

template <typename T>
[[nodiscard]]
auto
expand_socket_result(nix::scaredy_nix<T> result) -> scaredy<T, io_error> {
   if (result.has_value()) {
      if constexpr (is_void<T>) {
         return {};
      } else {
         return result.value();
      }
   }
   return static_cast<io_error>(-to_underlying(result.error()));
}

template <typename Flags>
[[nodiscard]]
constexpr auto
has_flag(Flags flags, Flags flag) -> bool {
   return (flags & flag) != Flags::none;
}

[[nodiscard]]
constexpr auto
accept_flags(socket_flags flags) -> nix::accept4_flags {
   nix::accept4_flags result = nix::accept4_flags::close_exec;
   if (has_flag(flags, socket_flags::nonblocking)) {
      result |= nix::accept4_flags::nonblocking;
   }
   return result;
}

[[nodiscard]]
constexpr auto
linux_domain(socket_domain domain) -> int8 {
   switch (domain) {
      case socket_domain::local:
         return 1;
      case socket_domain::ipv4:
         return 2;
      case socket_domain::ipv6:
         return 10;
   }
}

[[nodiscard]]
constexpr auto
linux_socket_type(socket_type type, socket_flags flags) -> int8 {
   uint4 result;
   switch (type) {
      case socket_type::stream:
         result = 1;
         break;
      case socket_type::datagram:
         result = 2;
         break;
      case socket_type::raw:
         result = 3;
         break;
      case socket_type::reliable_datagram:
         result = 4;
         break;
      case socket_type::sequenced_packet:
         result = 5;
         break;
      case socket_type::datagram_congestion_control:
         result = 6;
         break;
   }
   result |= 0x80000u;
   if (has_flag(flags, socket_flags::nonblocking)) {
      result |= 0x800u;
   }
   return result;
}

[[nodiscard]]
constexpr auto
linux_message_flags(socket_message_flags flags) -> nix::message_flags {
   nix::message_flags result = nix::message_flags::none;
   if (has_flag(flags, socket_message_flags::out_of_band)) {
      result |= nix::message_flags::out_of_band;
   }
   if (has_flag(flags, socket_message_flags::peek)) {
      result |= nix::message_flags::peek;
   }
   if (has_flag(flags, socket_message_flags::dont_route)) {
      result |= nix::message_flags::dont_route;
   }
   if (has_flag(flags, socket_message_flags::truncated)) {
      result |= nix::message_flags::truncated;
   }
   if (has_flag(flags, socket_message_flags::dont_wait)) {
      result |= nix::message_flags::dont_wait;
   }
   if (has_flag(flags, socket_message_flags::end_of_record)) {
      result |= nix::message_flags::end_of_record;
   }
   if (has_flag(flags, socket_message_flags::wait_all)) {
      result |= nix::message_flags::wait_all;
   }
   if (has_flag(flags, socket_message_flags::no_signal)) {
      result |= nix::message_flags::no_signal;
   }
   return result;
}

[[nodiscard]]
constexpr auto
socket_disposition(socket_flags flags) -> native_handle_disposition {
   native_handle_disposition result = native_handle_disposition::readable
                                      | native_handle_disposition::writable
                                      | native_handle_disposition::kernel_handle
                                      | native_handle_disposition::socket;
   if (has_flag(flags, socket_flags::nonblocking)) {
      result |= native_handle_disposition::nonblocking;
   }
   return result;
}

}  // namespace

unix_socket_address::unix_socket_address(file_path const& socket_path) {
   cat::assert(path_fits(socket_path));
   if (path_fits(socket_path)) {
      assign(socket_path);
   }
}

auto
unix_socket_address::create(file_path const& socket_path)
   -> scaredy<unix_socket_address, io_error> {
   if (!path_fits(socket_path)) {
      return io_error::filename_too_long;
   }
   unix_socket_address result;
   result.assign(socket_path);
   return result;
}

void
unix_socket_address::assign(file_path const& socket_path) {
   for (idx index = 0u; index < socket_path.size(); ++index) {
      m_path[index] = socket_path[index];
   }
   bool const abstract = !socket_path.empty() && socket_path[0u] == '\0';
   if (!abstract && !socket_path.empty()) {
      m_path[socket_path.size()] = '\0';
   }
   m_length = uint1(
      family_size + socket_path.size()
      + uint1(!abstract && !socket_path.empty())
   );
}

auto
socket_handle::operator=(socket_handle&& other) -> socket_handle& {
   if (this == __builtin_addressof(other)) {
      return *this;
   }
   auto _ = close();
   m_handle = other.release();
   return *this;
}

socket_handle::~socket_handle() {
   auto _ = close();
}

auto
socket_handle::create(
   socket_domain domain, socket_type type, socket_flags flags, int4 protocol
) -> scaredy<socket_handle, io_error> {
   nix::scaredy_nix<nix::file_descriptor> result = nix::sys_socket(
      linux_domain(domain), linux_socket_type(type, flags), protocol
   );
   if (result.has_value()) {
      nix::file_descriptor const descriptor = result.value();
      if (descriptor.value == nix::invalid_file_descriptor.value) {
         return adopt({});
      }
      return adopt(
         detail::native_handle_access::make(
            descriptor.value,
            socket_disposition(flags) | native_handle_disposition::kernel_handle
         )
      );
   }
   return static_cast<io_error>(-to_underlying(result.error()));
}

auto
socket_handle::close() -> scaredy<void, io_error> {
   if (!is_open()) {
      return {};
   }
   cat::native_handle const handle = release();
   iword const value = detail::native_handle_access::value(handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<void>(nix::sys_close(descriptor));
}

auto
socket_handle::bind_address(void const* _Nonnull p_address, iword size) const
   -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<void>(
      nix::sys_bind(descriptor, p_address, size)
   );
}

auto
socket_handle::bind(unix_socket_address const& address) const
   -> scaredy<void, io_error> {
   return bind_address(__builtin_addressof(address), address.address_size());
}

auto
socket_handle::bind(ipv4_socket_address const& address) const
   -> scaredy<void, io_error> {
   return bind_address(__builtin_addressof(address), sizeof(address));
}

auto
socket_handle::bind(ipv6_socket_address const& address) const
   -> scaredy<void, io_error> {
   return bind_address(__builtin_addressof(address), sizeof(address));
}

auto
socket_handle::bind(file_path const& socket_path) const
   -> scaredy<void, io_error> {
   auto address = unix_socket_address::create(socket_path);
   if (address.has_value()) {
      return bind(address.value());
   }
   return address.error();
}

auto
socket_handle::connect_address(void const* _Nonnull p_address, iword size) const
   -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<void>(
      nix::sys_connect(descriptor, p_address, size)
   );
}

auto
socket_handle::connect(unix_socket_address const& address) const
   -> scaredy<void, io_error> {
   return connect_address(__builtin_addressof(address), address.address_size());
}

auto
socket_handle::connect(ipv4_socket_address const& address) const
   -> scaredy<void, io_error> {
   return connect_address(__builtin_addressof(address), sizeof(address));
}

auto
socket_handle::connect(ipv6_socket_address const& address) const
   -> scaredy<void, io_error> {
   return connect_address(__builtin_addressof(address), sizeof(address));
}

auto
socket_handle::connect(file_path const& socket_path) const
   -> scaredy<void, io_error> {
   auto address = unix_socket_address::create(socket_path);
   if (address.has_value()) {
      return connect(address.value());
   }
   return address.error();
}

auto
socket_handle::listen(int8 backlog) const -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<void>(nix::sys_listen(descriptor, backlog));
}

auto
socket_handle::accept_address(
   void* _Nullable p_address, iword* _Nullable p_size, socket_flags flags
) const -> scaredy<socket_handle, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   nix::scaredy_nix<nix::file_descriptor> result =
      nix::sys_accept4(descriptor, p_address, p_size, accept_flags(flags));
   if (result.has_value()) {
      nix::file_descriptor const accepted = result.value();
      if (accepted.value == nix::invalid_file_descriptor.value) {
         return adopt({});
      }
      return adopt(
         detail::native_handle_access::make(
            accepted.value,
            socket_disposition(flags) | native_handle_disposition::kernel_handle
         )
      );
   }
   return static_cast<io_error>(-to_underlying(result.error()));
}

auto
socket_handle::accept4(socket_flags flags) const
   -> scaredy<socket_handle, io_error> {
   return accept_address(nullptr, nullptr, flags);
}

auto
socket_handle::accept4(unix_socket_address& address, socket_flags flags) const
   -> scaredy<socket_handle, io_error> {
   iword size = unix_socket_address::max_size;
   auto result = accept_address(__builtin_addressof(address), &size, flags);
   if (result.has_value()) {
      address.set_size(size);
   }
   return result;
}

auto
socket_handle::accept4(ipv4_socket_address& address, socket_flags flags) const
   -> scaredy<socket_handle, io_error> {
   iword size = sizeof(address);
   return accept_address(__builtin_addressof(address), &size, flags);
}

auto
socket_handle::accept4(ipv6_socket_address& address, socket_flags flags) const
   -> scaredy<socket_handle, io_error> {
   iword size = sizeof(address);
   return accept_address(__builtin_addressof(address), &size, flags);
}

auto
socket_handle::shutdown(socket_shutdown how) const -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<void>(
      nix::sys_shutdown(descriptor, static_cast<nix::shutdown_how>(how))
   );
}

auto
socket_handle::send(str_view buffer, socket_message_flags flags) const
   -> scaredy<idx, io_error> {
   return send_to_address(buffer, flags, nullptr, 0);
}

auto
socket_handle::receive(span<char> buffer, socket_message_flags flags) const
   -> scaredy<idx, io_error> {
   return receive_from_address(buffer, flags, nullptr, nullptr);
}

auto
socket_handle::send_to_address(
   str_view buffer, socket_message_flags flags, void const* _Nullable p_address,
   iword size
) const -> scaredy<idx, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<idx>(nix::sys_sendto(
      descriptor, buffer, linux_message_flags(flags), p_address, size
   ));
}

auto
socket_handle::send_to(
   str_view buffer, unix_socket_address const& address,
   socket_message_flags flags
) const -> scaredy<idx, io_error> {
   return send_to_address(
      buffer, flags, __builtin_addressof(address), address.address_size()
   );
}

auto
socket_handle::send_to(
   str_view buffer, ipv4_socket_address const& address,
   socket_message_flags flags
) const -> scaredy<idx, io_error> {
   return send_to_address(
      buffer, flags, __builtin_addressof(address), sizeof(address)
   );
}

auto
socket_handle::send_to(
   str_view buffer, ipv6_socket_address const& address,
   socket_message_flags flags
) const -> scaredy<idx, io_error> {
   return send_to_address(
      buffer, flags, __builtin_addressof(address), sizeof(address)
   );
}

auto
socket_handle::receive_from_address(
   span<char> buffer, socket_message_flags flags, void* _Nullable p_address,
   iword* _Nullable p_size
) const -> scaredy<idx, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<idx>(nix::sys_recv(
      descriptor, buffer, linux_message_flags(flags), p_address, p_size
   ));
}

auto
socket_handle::receive_from(
   span<char> buffer, unix_socket_address& address, socket_message_flags flags
) const -> scaredy<idx, io_error> {
   iword size = unix_socket_address::max_size;
   auto result =
      receive_from_address(buffer, flags, __builtin_addressof(address), &size);
   if (result.has_value()) {
      address.set_size(size);
   }
   return result;
}

auto
socket_handle::receive_from(
   span<char> buffer, ipv4_socket_address& address, socket_message_flags flags
) const -> scaredy<idx, io_error> {
   iword size = sizeof(address);
   return receive_from_address(
      buffer, flags, __builtin_addressof(address), &size
   );
}

auto
socket_handle::receive_from(
   span<char> buffer, ipv6_socket_address& address, socket_message_flags flags
) const -> scaredy<idx, io_error> {
   iword size = sizeof(address);
   return receive_from_address(
      buffer, flags, __builtin_addressof(address), &size
   );
}

auto
socket_handle::get_socket_name(void* _Nonnull p_address, iword& size) const
   -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<void>(
      nix::sys_getsockname(descriptor, p_address, size)
   );
}

auto
socket_handle::getsockname(unix_socket_address& address) const
   -> scaredy<void, io_error> {
   iword size = unix_socket_address::max_size;
   auto result = get_socket_name(__builtin_addressof(address), size);
   if (result.has_value()) {
      address.set_size(size);
   }
   return result;
}

auto
socket_handle::getsockname(ipv4_socket_address& address) const
   -> scaredy<void, io_error> {
   iword size = sizeof(address);
   return get_socket_name(__builtin_addressof(address), size);
}

auto
socket_handle::getsockname(ipv6_socket_address& address) const
   -> scaredy<void, io_error> {
   iword size = sizeof(address);
   return get_socket_name(__builtin_addressof(address), size);
}

auto
socket_handle::get_peer_name(void* _Nonnull p_address, iword& size) const
   -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_socket_result<void>(
      nix::sys_getpeername(descriptor, p_address, size)
   );
}

auto
socket_handle::getpeername(unix_socket_address& address) const
   -> scaredy<void, io_error> {
   iword size = unix_socket_address::max_size;
   auto result = get_peer_name(__builtin_addressof(address), size);
   if (result.has_value()) {
      address.set_size(size);
   }
   return result;
}

auto
socket_handle::getpeername(ipv4_socket_address& address) const
   -> scaredy<void, io_error> {
   iword size = sizeof(address);
   return get_peer_name(__builtin_addressof(address), size);
}

auto
socket_handle::getpeername(ipv6_socket_address& address) const
   -> scaredy<void, io_error> {
   iword size = sizeof(address);
   return get_peer_name(__builtin_addressof(address), size);
}

}  // namespace cat
