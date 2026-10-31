#include <cat/x11>

using cat::operator""_uki;

namespace {

// The bytes a parked reply of `size` bytes takes, including its header.
[[nodiscard]]
auto
parked_entry_size(cat::idx size) -> cat::idx {
   constexpr cat::idx header_size = sizeof(x11::detail::parked_reply);
   return header_size + (size + 7u) / 8u * 8u;
}

// A pending reply's sequence number, without its discarded bit.
[[nodiscard]]
auto
sequence_of(cat::uint8 pending) -> cat::uint8 {
   return pending & ~x11::detail::connection_state::discarded_bit;
}

void
complete_transfer(
   nix::io_uring_operation& operation, nix::io_uring_cqe const& completion
) noexcept {
   auto& transfer = static_cast<x11::detail::transfer_operation&>(operation);
   transfer.result = completion.result;
   transfer.is_complete = true;
}

void
complete_watch(
   nix::io_uring_operation& operation, nix::io_uring_cqe const& /*unused*/
) noexcept {
   auto& watch = static_cast<x11::detail::watch_operation&>(operation);
   watch.is_readable = true;
   watch.is_armed = false;
}

[[nodiscard]]
auto
parse_decimal(cat::str_view digits) -> cat::maybe<cat::uint4> {
   cat::idx const size = digits.size();
   if (size == 0u || size > 9u) {
      return cat::nullopt;
   }
   cat::uint4 value = 0u;
   for (char const character : digits) {
      if (character < '0' || character > '9') {
         return cat::nullopt;
      }
      // `character` is a digit, so this always fits.
      value =
         value * 10u + cat::narrow_cast<cat::uint4>(character - '0').value();
   }
   return value;
}

// The ASCII digit for the lowest decimal place of `value`.
[[nodiscard]]
auto
decimal_digit(cat::uint4 value) -> char {
   // A remainder of division by 10 always fits.
   return cat::narrow_cast<char>('0' + value % 10u).value();
}

[[nodiscard]]
auto
find_environment(
   char const* _Nullable const* _Nonnull p_environment, cat::str_view name
) -> cat::str_view {
   for (; *p_environment != nullptr; ++p_environment) {
      char const* p_entry = *p_environment;
      cat::idx i = 0u;
      // `p_entry` is null-terminated, so a mismatch stops this before then.
      while (i < name.size() && p_entry[i] == name.data()[i]) {
         ++i;
      }
      if (i == name.size() && p_entry[i] == '=') {
         char const* p_value = p_entry + i + 1u;
         return {p_value, cat::string_length(p_value)};
      }
   }
   return {"", 0u};
}

// Xauthority files store big-endian 16-bit lengths before each field.
[[nodiscard]]
auto
read_field(cat::span<cat::byte const> file, cat::idx& offset)
   -> cat::maybe<cat::str_view> {
   cat::idx const file_size = file.size();
   if (offset + 2u > file_size) {
      return cat::nullopt;
   }
   cat::idx const high = file[offset].value;
   cat::idx const low = file[offset + 1u].value;
   cat::idx const length = high * 256u + low;
   offset += 2u;
   if (offset + length > file_size) {
      return cat::nullopt;
   }
   cat::str_view const field(
      __builtin_bit_cast(char const*, file.data() + offset), length
   );
   offset += length;
   return field;
}

}  // namespace

auto
x11::parse_display_name(cat::str_view name) noexcept
   -> cat::maybe<display_name> {
   cat::idx const size = name.size();
   cat::idx colon = size;
   for (cat::idx i = 0u; i < size; ++i) {
      if (name.data()[i] == ':') {
         colon = i;
      }
   }
   if (colon == size) {
      return cat::nullopt;
   }

   display_name result{.host = cat::str_view(name.data(), colon)};
   char const* p_numbers = name.data() + colon + 1u;
   cat::idx const numbers_size = cat::idx(size - colon - 1u);
   cat::idx dot = numbers_size;
   for (cat::idx i = 0u; i < numbers_size; ++i) {
      if (p_numbers[i] == '.') {
         dot = i;
         break;
      }
   }
   result.display = $prop(parse_decimal(cat::str_view(p_numbers, dot)));
   if (dot != numbers_size) {
      result.screen = $prop(parse_decimal(
         cat::str_view(p_numbers + dot + 1u, cat::idx(numbers_size - dot - 1u))
      ));
   }
   return result;
}

auto
x11::find_authorization(
   cat::span<cat::byte const> file, cat::str_view host_name, cat::uint4 display
) noexcept -> cat::maybe<authorization> {
   constexpr cat::idx family_local = 256u;
   constexpr cat::idx family_wild = 65'535u;

   cat::array<char, 10u> display_digits;
   cat::idx digits_start = display_digits.size();
   cat::uint4 remaining = display;
   do {
      digits_start = cat::idx(digits_start - 1u);
      display_digits[digits_start] = decimal_digit(remaining);
      remaining /= 10u;
   } while (remaining != 0u);
   cat::str_view const display_string(
      display_digits.data() + digits_start,
      cat::idx(display_digits.size() - digits_start)
   );

   cat::idx const file_size = file.size();
   cat::idx offset = 0u;
   while (offset + 2u <= file_size) {
      cat::idx const family_high = file[offset].value;
      cat::idx const family_low = file[offset + 1u].value;
      cat::idx const family = family_high * 256u + family_low;
      offset += 2u;
      cat::str_view const address = $prop(read_field(file, offset));
      cat::str_view const number = $prop(read_field(file, offset));
      cat::str_view const name = $prop(read_field(file, offset));
      cat::str_view const data = $prop(read_field(file, offset));

      bool const host_matches =
         family == family_wild
         || (family == family_local && address == host_name);
      bool const display_matches =
         number.size() == 0u || number == display_string;
      if (
         host_matches && display_matches
         && name == cat::str_view("MIT-MAGIC-COOKIE-1")
      ) {
         return authorization{.name = name, .data = data};
      }
   }
   return cat::nullopt;
}

auto
x11::event::window() const noexcept -> x11::window {
   // The byte offset of each event's window field.
   cat::idx offset;
   switch (code()) {
      case event_code::key_press:
      case event_code::key_release:
      case event_code::button_press:
      case event_code::button_release:
      case event_code::motion_notify:
      case event_code::enter_notify:
      case event_code::leave_notify:
         offset = 12u;
         break;
      // These carry the parent or event window first, and the window they
      // are about second.
      case event_code::create_notify:
      case event_code::destroy_notify:
      case event_code::unmap_notify:
      case event_code::map_notify:
      case event_code::map_request:
      case event_code::reparent_notify:
      case event_code::configure_notify:
      case event_code::configure_request:
      case event_code::gravity_notify:
      case event_code::circulate_notify:
      case event_code::circulate_request:
         offset = 8u;
         break;
      case event_code::focus_in:
      case event_code::focus_out:
      case event_code::expose:
      case event_code::visibility_notify:
      case event_code::resize_request:
      case event_code::property_notify:
      case event_code::client_message:
         offset = 4u;
         break;
      default:
         return window::none;
   }
   x11::window result;
   __builtin_memcpy_inline(
      &result, __builtin_bit_cast(cat::byte const*, this) + offset, 4u
   );
   return result;
}

x11::connection::connection(connection&& other) noexcept
    : detail::connection_state(settled(other)) {
   // Only this connection owns the buffers and socket now.
   other.m_p_send = nullptr;
   other.m_p_receive = nullptr;
   other.m_p_events = nullptr;
   other.m_p_parked = nullptr;
   other.m_p_pending = nullptr;
   other.m_socket = nix::invalid_file_descriptor;
   other.m_has_socket_slot = false;
}

auto
x11::connection::operator=(connection&& other) noexcept -> connection& {
   if (this != &other) {
      this->~connection();
      new (this) connection(cat::move(other));
   }
   return *this;
}

x11::connection::~connection() {
   stop_watching();
   // Linux writes the send and receive buffers until their operations
   // complete, so a broken ring leaks them rather than free them early.
   bool const is_settled = settle_operations();
   finish_slot(false);
   if (is_settled) {
      if (m_p_send != nullptr) {
         m_allocator.free_multi(
            cat::span<cat::byte>(m_p_send, m_send_capacity)
         );
      }
      if (m_p_receive != nullptr) {
         m_allocator.free_multi(
            cat::span<cat::byte>(m_p_receive, m_receive_capacity)
         );
      }
      if (m_has_socket_slot) {
         auto _ = m_p_ring->unregister_file(m_socket_slot);
      }
   }
   if (m_p_events != nullptr) {
      m_allocator.free_multi(
         cat::span<cat::byte>(m_p_events, m_event_capacity)
      );
   }
   if (m_p_parked != nullptr) {
      m_allocator.free_multi(
         cat::span<cat::byte>(m_p_parked, m_parked_capacity)
      );
   }
   if (m_p_pending != nullptr) {
      m_allocator.free_multi(
         cat::span<cat::uint8>(m_p_pending, m_pending_capacity)
      );
   }
   if (m_socket.value != nix::invalid_file_descriptor.value) {
      auto _ = nix::sys_close(m_socket);
   }
}

void
x11::connection::fail(error_code code) noexcept {
   if (!m_has_failed) {
      m_failure = {};
      m_failure.code = code;
      m_failure.sequence = detail::low_sequence(m_sequence);
      m_has_failed = true;
   }
   // Nothing more can be sent.
   m_send_size = 0u;
   m_send_submitted = 0u;
}

void
x11::connection::fail(nix::linux_error linux_error) noexcept {
   if (!m_has_failed) {
      m_failure = error_packet::from_linux(linux_error);
      m_has_failed = true;
   }
   m_send_size = 0u;
   m_send_submitted = 0u;
}

void
x11::connection::reject(error_code code, cat::idx bad_value) noexcept {
   if (m_has_failed) {
      m_unsent = m_failure;
      return;
   }
   m_unsent = {};
   m_unsent.code = code;
   m_unsent.bad_value = cat::narrow_cast<cat::uint4>(bad_value).value_or(
      cat::limits<cat::uint4>::max()
   );
}

auto
x11::connection::open(
   cat::allocator_dyn allocator,
   char const* _Nullable const* _Nonnull p_environment
) noexcept -> scaredy_x11<connection> {
   cat::str_view const display = find_environment(p_environment, "DISPLAY");

   // Build a null-terminated path to the Xauthority file.
   cat::array<char, 4_uki> path;
   cat::idx path_size = 0u;
   auto const append = [&](cat::str_view part) {
      cat::idx const part_size = part.size();
      cat::idx const size = path_size + part_size < path.size()
                               ? part_size
                               : cat::idx(path.size() - 1u - path_size);
      __builtin_memcpy(path.data() + path_size, part.data(), size);
      path_size += size;
   };
   cat::str_view const authority =
      find_environment(p_environment, "XAUTHORITY");
   if (authority.size() != 0u) {
      append(authority);
   } else {
      cat::str_view const home = find_environment(p_environment, "HOME");
      if (home.size() != 0u) {
         append(home);
         append("/.Xauthority");
      }
   }
   path[path_size] = '\0';
   return open(allocator, display, cat::zstr_view(path.data(), path_size + 1u));
}

auto
x11::connection::open(
   cat::allocator_dyn allocator, cat::str_view display,
   cat::zstr_view authority_path
) noexcept -> scaredy_x11<connection> {
   cat::maybe<display_name> const parsed = parse_display_name(display);
   if (!parsed.has_value()) {
      return error_packet::from_linux(nix::linux_error::inval);
   }
   // TCP displays are not supported yet. Connecting to one only needs a TCP
   // socket here, and `FamilyInternet` host matching in `find_authorization()`.
   if (
      parsed.value().host.size() != 0u
      && !(parsed.value().host == cat::str_view("unix"))
   ) {
      return error_packet::from_linux(nix::linux_error::afnosupport);
   }

   nix::io_uring* p_ring = nix::io_uring_runtime();
   if (p_ring == nullptr) {
      return error_packet::from_linux(nix::linux_error::badfd);
   }
   connection result(allocator);
   result.m_p_ring = p_ring;

   auto send =
      allocator.template alloc_multi_uninit<cat::byte>(initial_send_capacity);
   auto receive = allocator.template alloc_multi_uninit<cat::byte>(
      initial_receive_capacity
   );
   auto events =
      allocator.template alloc_multi_uninit<cat::byte>(initial_event_capacity);
   if (send.has_value()) {
      result.m_p_send = send.value().data();
      result.m_send_capacity = initial_send_capacity;
   }
   if (receive.has_value()) {
      result.m_p_receive = receive.value().data();
      result.m_receive_capacity = initial_receive_capacity;
   }
   if (events.has_value()) {
      result.m_p_events = events.value().data();
      result.m_event_capacity = initial_event_capacity;
   }
   auto pending = allocator.template alloc_multi_uninit<cat::uint8>(
      initial_pending_capacity
   );
   if (pending.has_value()) {
      result.m_p_pending = pending.value().data();
      result.m_pending_capacity = initial_pending_capacity;
   }
   if (
      !send.has_value() || !receive.has_value() || !events.has_value()
      || !pending.has_value()
   ) {
      return error_packet::from_linux(nix::linux_error::nomem);
   }

   // Find this display's cookie. The receive buffer holds the Xauthority file
   // until the handshake, so `authority` views it until then.
   authorization authority{};
   if (authority_path.size() > 1u) {
      auto file = nix::sys_open(authority_path, nix::open_mode::read_only);
      if (file.has_value()) {
         auto read = nix::sys_read(
            file.value(), cat::span<char>(
                             __builtin_bit_cast(char*, result.m_p_receive),
                             result.m_receive_capacity
                          )
         );
         auto _ = nix::sys_close(file.value());
         nix::utsname host{};
         if (read.has_value() && nix::sys_uname(host).has_value()) {
            char const* p_host_name = host.nodename.data();
            authority =
               find_authorization(
                  cat::span<cat::byte const>(result.m_p_receive, read.value()),
                  cat::str_view(p_host_name, cat::string_length(p_host_name)),
                  parsed.value().display
               )
                  .value_or(authorization{});
         }
      }
   }

   auto socket = nix::create_socket_local(
      // `SOCK_STREAM | SOCK_CLOEXEC`
      1 | 0x80000, 0
   );
   if (socket.is_empty()) {
      return error_packet::from_linux(socket.error());
   }
   result.m_socket = socket.value();

   // `sockaddr_un` for `/tmp/.X11-unix/X<display>`.
   struct {
      cat::uint2 family = 1u;
      cat::array<char, 108u> path{};
   } address;

   __builtin_memcpy_inline(address.path.data(), "/tmp/.X11-unix/X", 16u);
   cat::idx path_size = 16u;
   cat::array<char, 10u> digits;
   cat::idx digits_size = 0u;
   cat::uint4 remaining = parsed.value().display;
   do {
      digits[digits_size] = decimal_digit(remaining);
      ++digits_size;
      remaining /= 10u;
   } while (remaining != 0u);
   while (digits_size != 0u) {
      digits_size = cat::idx(digits_size - 1u);
      address.path[path_size] = digits[digits_size];
      ++path_size;
   }
   // The family, the path, and its null terminator.
   auto const address_size = cat::narrow_cast<cat::iword>(path_size + 3u);
   if (!address_size.has_value()) {
      return error_packet::from_linux(nix::linux_error::nametoolong);
   }
   auto connected =
      nix::sys_connect(result.m_socket, &address, address_size.value());
   if (connected.is_empty()) {
      return error_packet::from_linux(connected.error());
   }

   $prop(result.handshake(authority, parsed.value().screen));
   return cat::move(result);
}

auto
x11::connection::handshake(
   authorization authority, cat::uint4 screen_index
) noexcept -> scaredy_x11<void> {
   // Xauthority fields hold 16-bit lengths, so a parsed cookie fits.
   cat::maybe<cat::uint2> const name_length =
      cat::narrow_cast<cat::uint2>(authority.name.size());
   cat::maybe<cat::uint2> const data_length =
      cat::narrow_cast<cat::uint2>(authority.data.size());
   if (!name_length.has_value() || !data_length.has_value()) {
      fail(nix::linux_error::inval);
      return m_failure;
   }

   struct {
      char byte_order = cat::is_target_little_endian ? 'l' : 'B';
      cat::uint1 _ = 0u;
      cat::uint2 major_version = 11u;
      cat::uint2 minor_version = 0u;
      cat::uint2 name_length;
      cat::uint2 data_length;
      cat::uint2 _ = 0u;
   } const request{
      .name_length = name_length.value(), .data_length = data_length.value()
   };

   static_assert(sizeof(request) == 12);

   // The handshake is not a request, so this bypasses `begin_request()`.
   cat::idx const name_size = pad(authority.name.size());
   cat::idx const data_size = pad(authority.data.size());
   constexpr cat::idx request_size = sizeof(request);
   cat::idx const setup_request_size = request_size + name_size + data_size;
   if (
      setup_request_size > m_send_capacity
      && !grow(m_p_send, m_send_capacity, setup_request_size)
   ) {
      return m_failure;
   }
   cat::byte* p_send = m_p_send;
   __builtin_memset(p_send, 0, setup_request_size);
   __builtin_memcpy_inline(p_send, &request, sizeof(request));
   __builtin_memcpy(
      p_send + request_size, authority.name.data(), authority.name.size()
   );
   __builtin_memcpy(
      p_send + sizeof(request) + name_size, authority.data.data(),
      authority.data.size()
   );
   m_send_size = setup_request_size;
   // `authority` viewed the receive buffer, which receiving overwrites.
   m_receive_tail = 0u;

   // Requests may follow the setup request without waiting for its reply, so
   // the window manager atoms share the setup's round trip.
   cookie const wm_protocols = intern_atom("WM_PROTOCOLS");
   cookie const wm_delete_window = intern_atom("WM_DELETE_WINDOW");
   cookie const net_wm_name = intern_atom("_NET_WM_NAME");
   cookie const utf8_string = intern_atom("UTF8_STRING");

   // Every setup response begins with 8 bytes that hold its length in 4-byte
   // units.
   auto const setup_size = [&] -> cat::idx {
      cat::uint2 words;
      __builtin_memcpy_inline(&words, m_p_receive + 6, 2u);
      cat::idx const wide_words = words;
      return wide_words * 4u + 8u;
   };
   while (m_receive_tail < 8u || m_receive_tail < setup_size()) {
      if (!transfer(transfer_mode::receive)) {
         return m_failure;
      }
   }

   cat::byte const* p_setup = m_p_receive;
   m_receive_head = setup_size();
   if (p_setup[0] != 1u) {
      // 0 is a refusal, and 2 asks for further authentication, which only the
      // cookie scheme here could provide.
      fail(
         p_setup[0] == 0u ? nix::linux_error::connrefused
                          : nix::linux_error::acces
      );
      return m_failure;
   }
   cat::idx const setup_size_value = m_receive_head;

   struct {
      cat::uint4 release_number;
      cat::uint4 resource_id_base;
      cat::uint4 resource_id_mask;
      cat::uint4 motion_buffer_size;
      cat::uint2 vendor_length;
      cat::uint2 maximum_request_length;
      cat::uint1 screens_count;
      cat::uint1 formats_count;
      cat::uint1 _[10];  // NOLINT
   } info;

   static_assert(sizeof(info) == 32);
   constexpr cat::idx info_size = sizeof(info);
   if (setup_size_value < info_size + 8u) {
      fail(nix::linux_error::proto);
      return m_failure;
   }
   __builtin_memcpy_inline(&info, p_setup + 8, sizeof(info));
   m_id_base = info.resource_id_base;
   m_id_mask = info.resource_id_mask;
   m_id_increment = info.resource_id_mask & (~info.resource_id_mask + 1u);
   cat::idx const maximum_request_words = info.maximum_request_length;
   m_maximum_request_size = maximum_request_words * 4u;

   cat::idx const vendor_size = info.vendor_length;
   cat::idx const formats_count = info.formats_count;
   cat::idx offset = info_size + 8u + pad(vendor_size) + formats_count * 8u;
   if (screen_index >= info.screens_count) {
      fail(nix::linux_error::inval);
      return m_failure;
   }
   for (cat::uint4 i = 0u;; ++i) {
      struct {
         x11::window root;
         x11::colormap default_colormap;
         cat::uint4 white_pixel, black_pixel;
         cat::uint4 current_input_masks;
         cat::uint2 width, height;
         cat::uint2 width_millimeters, height_millimeters;
         cat::uint2 minimum_maps, maximum_maps;
         x11::visual root_visual;
         cat::uint1 backing_stores;
         cat::uint1 save_unders;
         cat::uint1 root_depth;
         cat::uint1 depths_count;
      } screen_info;

      static_assert(sizeof(screen_info) == 40);
      constexpr cat::idx screen_info_size = sizeof(screen_info);
      if (offset + screen_info_size > setup_size_value) {
         fail(nix::linux_error::proto);
         return m_failure;
      }
      __builtin_memcpy_inline(
         &screen_info, p_setup + offset, sizeof(screen_info)
      );
      offset += screen_info_size;
      if (i == screen_index) {
         m_screen = {
            .root = screen_info.root,
            .default_colormap = screen_info.default_colormap,
            .white_pixel = screen_info.white_pixel,
            .black_pixel = screen_info.black_pixel,
            .width = screen_info.width,
            .height = screen_info.height,
            .root_visual = screen_info.root_visual,
            .root_depth = screen_info.root_depth,
         };
         break;
      }
      // Skip each depth, and the 24-byte visuals that follow it.
      for (cat::uint1 depth = 0u; depth < screen_info.depths_count; ++depth) {
         if (offset + 8u > setup_size_value) {
            fail(nix::linux_error::proto);
            return m_failure;
         }
         cat::uint2 visuals_count;
         __builtin_memcpy_inline(&visuals_count, p_setup + offset + 2, 2u);
         cat::idx const wide_visuals_count = visuals_count;
         offset += wide_visuals_count * 24u + 8u;
      }
   }

   m_wm_protocols = $prop(wait(wm_protocols)).atom;
   m_wm_delete_window = $prop(wait(wm_delete_window)).atom;
   m_net_wm_name = $prop(wait(net_wm_name)).atom;
   m_utf8_string = $prop(wait(utf8_string)).atom;
   return cat::monostate;
}

auto
x11::connection::transfer(transfer_mode mode) noexcept -> bool {
   if (m_has_failed) {
      return false;
   }
   // The rest of the send buffer must follow the chunk being sent.
   if (m_has_chunk && !finish_chunk()) {
      return false;
   }
   cat::idx const send_size = cat::idx(m_send_size - m_send_submitted);
   bool const is_sending = send_size != 0u;
   bool const is_receiving = mode != transfer_mode::send;
   if (!is_sending && !is_receiving) {
      return true;
   }

   // Make room to receive before reserving any SQE, so that failing here
   // leaves nothing in the shared ring.
   if (is_receiving) {
      // Responses before `m_receive_head` have been consumed, so the bytes
      // after it can move to the front.
      if (m_receive_head != 0u) {
         cat::idx const remaining = cat::idx(m_receive_tail - m_receive_head);
         __builtin_memmove(
            m_p_receive, m_p_receive + m_receive_head, remaining
         );
         m_receive_head = 0u;
         m_receive_tail = remaining;
      }
      // A full buffer holds part of one large response.
      if (
         m_receive_tail == m_receive_capacity
         && !grow(m_p_receive, m_receive_capacity, m_receive_capacity + 1u)
      ) {
         return false;
      }
   }

   // Lengths in SQEs hold 32 bits. Sends are bounded by the largest request,
   // and receiving less than a huge buffer's capacity is harmless.
   auto const send_length = cat::narrow_cast<cat::uint4>(send_size);
   cat::uint4 const receive_length =
      cat::narrow_cast<cat::uint4>(m_receive_capacity - m_receive_tail)
         .value_or(cat::limits<cat::uint4>::max());
   auto const socket = cat::narrow_cast<cat::int4>(m_socket.value);
   if (!send_length.has_value() || !socket.has_value()) {
      fail(nix::linux_error::msgsize);
      return false;
   }
   cat::int4 const socket_value = socket.value();

   // These SQEs must be submitted together, or their link would be split.
   nix::io_uring_submission_queue& submissions = m_p_ring->submission_queue();
   if (submissions.space_left() < 4u) {
      auto _ = m_p_ring->submit();
      if (submissions.space_left() < 4u) {
         fail(nix::linux_error::busy);
         return false;
      }
   }

   // Registering the socket in a fixed file slot lets later submissions skip
   // the descriptor table lookup, and riding along with this submission saves
   // a system call of its own.
   if (!m_has_socket_slot && !m_is_slot_pending && !m_is_slot_unavailable) {
      nix::io_uring_sqe* p_update = submissions.get_sqe();
      m_slot_value = socket_value;
      m_slot_operation = {{complete_transfer}, 0, false};
      m_is_slot_pending = true;
      p_update->opcode = nix::io_uring_opcode::files_update;
      p_update->offset = nix::io_uring_file_index_alloc;
      p_update->address = __builtin_bit_cast(cat::uword, &m_slot_value);
      p_update->length = 1u;
      p_update->user_data = __builtin_bit_cast(cat::uword, &m_slot_operation);
   }
   // A poll which finds nothing leaves this watching the socket, so that
   // later idle polls need no system call until it becomes readable.
   if (mode == transfer_mode::poll && !m_watch_operation.is_armed) {
      nix::io_uring_sqe* p_poll = submissions.get_sqe();
      p_poll->opcode = nix::io_uring_opcode::poll_add;
      set_socket(*p_poll);
      // `POLLIN`. A socket that is readable already completes this at once.
      p_poll->poll_events_32 = 1u;
      p_poll->user_data = __builtin_bit_cast(cat::uword, &m_watch_operation);
      m_watch_operation = {{complete_watch}, true, false};
   }
   m_send_operation = {{complete_transfer}, 0, !is_sending};
   m_receive_operation = {{complete_transfer}, 0, !is_receiving};

   if (is_sending) {
      nix::io_uring_sqe* p_send = submissions.get_sqe();
      p_send->opcode = nix::io_uring_opcode::send;
      // Linking defers the receive until the send has finished, and cancels
      // it if the send fails, so this never waits for a reply to a request
      // that was not sent.
      set_socket(*p_send);
      if (is_receiving) {
         p_send->flags |= nix::io_uring_sqe_flags::io_link;
      }
      p_send->address =
         __builtin_bit_cast(cat::idx, m_p_send + m_send_submitted);
      p_send->length = send_length.value();
      // `MSG_WAITALL` makes the ring retry partial sends.
      p_send->message_flags = cat::to_underlying(
         nix::message_flags::wait_all | nix::message_flags::no_signal
      );
      p_send->user_data = __builtin_bit_cast(cat::uword, &m_send_operation);
   }

   if (is_receiving) {
      nix::io_uring_sqe* p_receive = submissions.get_sqe();
      p_receive->opcode = nix::io_uring_opcode::recv;
      set_socket(*p_receive);
      p_receive->address =
         __builtin_bit_cast(cat::idx, m_p_receive + m_receive_tail);
      p_receive->length = receive_length;
      p_receive->message_flags =
         mode == transfer_mode::poll
            ? cat::to_underlying(nix::message_flags::dont_wait)
            : 0u;
      p_receive->user_data =
         __builtin_bit_cast(cat::uword, &m_receive_operation);
   }

   // Submit and wait in one system call. Completions of other operations on
   // the shared ring are dispatched to their owners along the way.
   if (!settle_operations()) {
      return false;
   }
   finish_slot(false);

   nix::linux_error failure = nix::linux_error::none;
   if (is_sending) {
      cat::int4 const result = m_send_operation.result;
      if (result < 0) {
         failure = static_cast<nix::linux_error>(result);
      } else if (cat::narrow_cast<cat::idx>(result).value() != send_size) {
         failure = nix::linux_error::io;
      }
   }
   if (is_receiving) {
      cat::int4 const result = m_receive_operation.result;
      if (result > 0) {
         m_receive_tail += cat::narrow_cast<cat::idx>(result).value();
      } else if (result == 0) {
         // The server closed the connection.
         failure = nix::linux_error::connreset;
      } else if (
         result != cat::to_underlying(nix::linux_error::again)
         && failure == nix::linux_error::none
      ) {
         failure = static_cast<nix::linux_error>(result);
      }
   }

   if (failure != nix::linux_error::none) {
      fail(failure);
      return false;
   }
   m_send_size = 0u;
   m_send_submitted = 0u;
   return true;
}

auto
x11::connection::settle_operations() noexcept -> bool {
   for (;;) {
      // Counting every operation in flight keeps any of their completions
      // from ending the wait early.
      cat::uint4 const in_flight = (m_send_operation.is_complete ? 0u : 1u)
                                   + (m_receive_operation.is_complete ? 0u : 1u)
                                   + (m_slot_operation.is_complete ? 0u : 1u)
                                   + (m_chunk_operation.is_complete ? 0u : 1u)
                                   + (m_cancel_operation.is_complete ? 0u : 1u);
      if (in_flight == 0u) {
         return true;
      }
      if (m_is_ring_broken) {
         return false;
      }
      auto dispatched = m_p_ring->dispatch(in_flight);
      if (dispatched.is_empty()) {
         m_is_ring_broken = true;
         fail(dispatched.error());
         return false;
      }
   }
}

auto
x11::connection::make_send_space(cat::idx size) noexcept -> bool {
   if (!transfer(transfer_mode::send)) {
      return false;
   }
   return size <= m_send_capacity || grow(m_p_send, m_send_capacity, size);
}

auto
x11::connection::grow(
   cat::byte*& p_buffer, cat::idx& capacity, cat::idx minimum
) noexcept -> bool {
   cat::idx new_capacity = capacity == 0u ? minimum : capacity;
   while (new_capacity < minimum) {
      new_capacity = new_capacity * 2u;
   }
   auto grown =
      p_buffer == nullptr
         ? m_allocator.template alloc_multi_uninit<cat::byte>(new_capacity)
         : m_allocator.template realloc_multi_uninit<cat::byte>(
              p_buffer, capacity, new_capacity
           );
   if (!grown.has_value()) {
      fail(nix::linux_error::nomem);
      return false;
   }
   p_buffer = grown.value().data();
   capacity = new_capacity;
   return true;
}

auto
x11::connection::make_pending_space() noexcept -> bool {
   cat::idx const count = cat::idx(m_pending_tail - m_pending_head);
   if (m_pending_head != 0u) {
      __builtin_memmove(
         m_p_pending, m_p_pending + m_pending_head, count * sizeof(cat::uint8)
      );
      m_pending_head = 0u;
      m_pending_tail = count;
      return true;
   }
   auto grown = m_allocator.template realloc_multi_uninit<cat::uint8>(
      m_p_pending, m_pending_capacity, m_pending_capacity * 2u
   );
   if (!grown.has_value()) {
      fail(nix::linux_error::nomem);
      return false;
   }
   m_p_pending = grown.value().data();
   m_pending_capacity = m_pending_capacity * 2u;
   return true;
}

auto
x11::connection::find_pending(cat::uint8 sequence) const noexcept -> cat::idx {
   // The pending replies are in increasing order, so search them in halves.
   cat::idx low = m_pending_head;
   cat::idx high = m_pending_tail;
   while (low < high) {
      cat::idx const middle = cat::idx(low + cat::idx(high - low) / 2u);
      if (sequence_of(m_p_pending[middle]) < sequence) {
         low = cat::idx(middle + 1u);
      } else {
         high = middle;
      }
   }
   if (low < m_pending_tail && sequence_of(m_p_pending[low]) == sequence) {
      return low;
   }
   return m_pending_tail;
}

void
x11::connection::send_chunk() noexcept {
   if (m_has_failed) {
      return;
   }
   if (m_has_chunk) {
      // Collect completions that are already posted, which needs no system
      // call. Sends that fit in the socket buffer complete during submission.
      auto _ = m_p_ring->dispatch(0u);
      if (!m_chunk_operation.is_complete) {
         // A send that waited for socket space completes only once this
         // thread enters the ring, because of `DEFER_TASKRUN`.
         auto _ =
            m_p_ring->enter(0u, 0u, nix::io_uring_enter_flags::get_events);
         auto _ = m_p_ring->dispatch(0u);
      }
      if (!m_chunk_operation.is_complete) {
         // A stream socket needs its sends in order, so wait for a later
         // chunk or the next transfer.
         return;
      }
      if (!finish_chunk()) {
         return;
      }
   }
   nix::io_uring_sqe* p_send = m_p_ring->submission_queue().get_sqe();
   if (p_send == nullptr) {
      return;
   }
   m_chunk_length = cat::idx(m_send_size - m_send_submitted);
   auto const chunk_length = cat::narrow_cast<cat::uint4>(m_chunk_length);
   if (!chunk_length.has_value()) {
      return;
   }
   p_send->opcode = nix::io_uring_opcode::send;
   set_socket(*p_send);
   p_send->address = __builtin_bit_cast(cat::idx, m_p_send + m_send_submitted);
   p_send->length = chunk_length.value();
   p_send->message_flags = cat::to_underlying(
      nix::message_flags::wait_all | nix::message_flags::no_signal
   );
   p_send->user_data = __builtin_bit_cast(cat::uword, &m_chunk_operation);
   m_chunk_operation = {{complete_transfer}, 0, false};
   m_send_submitted = m_send_size;
   m_has_chunk = true;
   // An interrupted or busy submission leaves the chunk for the next one.
   auto submitted = m_p_ring->submit();
   if (
      submitted.is_empty() && submitted.error() != nix::linux_error::intr
      && submitted.error() != nix::linux_error::again
      && submitted.error() != nix::linux_error::busy
   ) {
      m_is_ring_broken = true;
      fail(submitted.error());
   }
}

auto
x11::connection::finish_chunk() noexcept -> bool {
   if (!settle_operations()) {
      return false;
   }
   m_has_chunk = false;
   cat::int4 const result = m_chunk_operation.result;
   if (result < 0) {
      fail(static_cast<nix::linux_error>(result));
   } else if (cat::narrow_cast<cat::idx>(result).value() != m_chunk_length) {
      fail(nix::linux_error::io);
   }
   return !m_has_failed;
}

void
x11::connection::finish_slot(bool wait) noexcept {
   if (wait && !settle_operations()) {
      return;
   }
   if (m_is_slot_pending && m_slot_operation.is_complete) {
      m_is_slot_pending = false;
      if (m_slot_operation.result < 0) {
         // The table is full, so keep using the descriptor.
         m_is_slot_unavailable = true;
      } else {
         // Linux wrote a slot index over the descriptor.
         m_socket_slot = cat::narrow_cast<cat::uint4>(m_slot_value).value();
         m_has_socket_slot = true;
      }
   }
}

void
x11::connection::set_socket(nix::io_uring_sqe& submission) const noexcept {
   if (m_has_socket_slot) {
      submission.flags = nix::io_uring_sqe_flags::fixed_file;
      submission.file_descriptor =
         cat::narrow_cast<cat::int4>(m_socket_slot).value();
   } else {
      submission.flags = nix::io_uring_sqe_flags::none;
      submission.file_descriptor =
         cat::narrow_cast<cat::int4>(m_socket.value).value();
   }
}

void
x11::connection::stop_watching() noexcept {
   if (!m_watch_operation.is_armed || m_is_ring_broken) {
      return;
   }
   nix::io_uring_submission_queue& submissions = m_p_ring->submission_queue();
   nix::io_uring_sqe* p_cancel = submissions.get_sqe();
   if (p_cancel == nullptr) {
      auto _ = m_p_ring->submit();
      p_cancel = submissions.get_sqe();
      if (p_cancel == nullptr) {
         m_is_ring_broken = true;
         return;
      }
   }
   p_cancel->opcode = nix::io_uring_opcode::async_cancel;
   p_cancel->address = __builtin_bit_cast(cat::uword, &m_watch_operation);
   p_cancel->user_data = __builtin_bit_cast(cat::uword, &m_cancel_operation);
   m_cancel_operation = {{complete_transfer}, 0, false};
   while (m_watch_operation.is_armed || !m_cancel_operation.is_complete) {
      if (m_p_ring->dispatch(1u).is_empty()) {
         m_is_ring_broken = true;
         return;
      }
   }
}

auto
x11::connection::settled(connection& other) noexcept -> connection& {
   other.stop_watching();
   other.finish_slot(true);
   return other;
}

auto
x11::connection::next_response() noexcept -> cat::byte const* _Nullable {
   cat::idx const available = cat::idx(m_receive_tail - m_receive_head);
   if (available < 32u) {
      return nullptr;
   }
   cat::byte const* p_response = m_p_receive + m_receive_head;
   cat::idx const size = response_size(p_response);
   if (available < size) {
      return nullptr;
   }
   m_receive_head += size;

   // `keymap_notify` is the only response without a sequence number.
   if (p_response[0] != cat::to_underlying(event_code::keymap_notify)) {
      cat::uint2 sequence;
      __builtin_memcpy_inline(&sequence, p_response + 2, 2u);
      cat::uint8 const wide_sequence = sequence;
      m_last_response_sequence +=
         (wide_sequence - m_last_response_sequence).wrap() & 0xffffu;
   }
   return p_response;
}

auto
x11::connection::queue_event(cat::byte const* _Nonnull p_response) noexcept
   -> bool {
   cat::idx const size = response_size(p_response);
   if (m_event_tail + size > m_event_capacity) {
      cat::idx const remaining = cat::idx(m_event_tail - m_event_head);
      __builtin_memmove(m_p_events, m_p_events + m_event_head, remaining);
      m_event_head = 0u;
      m_event_tail = remaining;
      if (
         m_event_tail + size > m_event_capacity
         && !grow(m_p_events, m_event_capacity, m_event_tail + size)
      ) {
         return false;
      }
   }
   __builtin_memcpy(m_p_events + m_event_tail, p_response, size);
   m_event_tail += size;
   return true;
}

auto
x11::connection::find_parked(cat::uint8 sequence) const noexcept -> cat::idx {
   cat::idx offset = 0u;
   while (offset < m_parked_size) {
      detail::parked_reply entry;
      __builtin_memcpy_inline(&entry, m_p_parked + offset, sizeof(entry));
      if (entry.sequence == sequence) {
         return offset;
      }
      offset += parked_entry_size(entry.size);
   }
   return m_parked_size;
}

void
x11::connection::erase_parked(cat::idx offset) noexcept {
   detail::parked_reply entry;
   __builtin_memcpy_inline(&entry, m_p_parked + offset, sizeof(entry));
   cat::idx const size = parked_entry_size(entry.size);
   cat::idx const after = offset + size;
   __builtin_memmove(
      m_p_parked + offset, m_p_parked + after, cat::idx(m_parked_size - after)
   );
   m_parked_size = cat::idx(m_parked_size - size);
}

void
x11::connection::release_taken() noexcept {
   if (m_has_taken_parked) {
      m_has_taken_parked = false;
      erase_parked(m_taken_parked);
   }
}

auto
x11::connection::route(
   cat::byte const* _Nonnull p_response, cat::uint8 target
) noexcept -> routed {
   cat::uint8 const sequence = m_last_response_sequence;
   bool is_reply_request = p_response[0] == 1u;
   bool is_discarded = false;
   // Every request with a reply gets exactly one reply or error, in request
   // order, so pending replies before this response are never coming.
   while (m_pending_head != m_pending_tail) {
      cat::uint8 const pending = m_p_pending[m_pending_head];
      if (sequence_of(pending) > sequence) {
         break;
      }
      ++m_pending_head;
      if (sequence_of(pending) == sequence) {
         is_reply_request = true;
         is_discarded = pending != sequence_of(pending);
         break;
      }
   }
   if (m_pending_head == m_pending_tail) {
      m_pending_head = 0u;
      m_pending_tail = 0u;
   }

   if (is_reply_request) {
      if (is_discarded) {
         return routed::kept;
      }
      if (sequence == target) {
         return routed::target;
      }
      return park(p_response) ? routed::kept : routed::failed;
   }
   if (sequence == m_check_sequence && !m_has_checked) {
      __builtin_memcpy_inline(&m_checked, p_response, sizeof(m_checked));
      m_has_checked = true;
      return routed::kept;
   }
   return routed::event;
}

auto
x11::connection::park(cat::byte const* _Nonnull p_response) noexcept -> bool {
   detail::parked_reply const entry{
      .sequence = m_last_response_sequence,
      .size = response_size(p_response),
   };
   cat::idx const size = parked_entry_size(entry.size);
   if (
      m_parked_size + size > m_parked_capacity
      && !grow(m_p_parked, m_parked_capacity, m_parked_size + size)
   ) {
      return false;
   }
   __builtin_memcpy_inline(m_p_parked + m_parked_size, &entry, sizeof(entry));
   __builtin_memcpy(
      m_p_parked + m_parked_size + sizeof(entry), p_response, entry.size
   );
   m_parked_size += size;
   return true;
}

void
x11::connection::discard_sequence(cat::uint8 sequence) noexcept {
   // Erasing a parked reply moves the ones after it, including the one the
   // last wait returned, so release that first.
   release_taken();
   cat::idx const parked = find_parked(sequence);
   if (parked != m_parked_size) {
      erase_parked(parked);
      return;
   }
   // Mark a reply that has not arrived, so that `route()` drops it.
   cat::idx const pending = find_pending(sequence);
   if (pending != m_pending_tail) {
      m_p_pending[pending] = sequence | detail::connection_state::discarded_bit;
   }
}

auto
x11::connection::wait_response(cat::uint8 sequence) noexcept
   -> cat::byte const* _Nonnull {
   release_taken();
   if (sequence == 0u) {
      return __builtin_bit_cast(cat::byte const*, &m_unsent);
   }
   cat::idx const parked = find_parked(sequence);
   if (parked != m_parked_size) {
      // This stays valid until the next call releases it.
      m_taken_parked = parked;
      m_has_taken_parked = true;
      return m_p_parked + parked + sizeof(detail::parked_reply);
   }
   cat::idx const pending = find_pending(sequence);
   if (
      pending == m_pending_tail
      || m_p_pending[pending] != sequence_of(m_p_pending[pending])
   ) {
      // Its response was already waited or discarded, so none is coming.
      m_misuse = error_packet::from_linux(nix::linux_error::noent);
      return __builtin_bit_cast(cat::byte const*, &m_misuse);
   }

   for (;;) {
      // Responses that already arrived are served even after a failure.
      while (cat::byte const* p_response = next_response()) {
         if (p_response[0].value > 1u) {
            if (!queue_event(p_response)) {
               break;
            }
            continue;
         }
         routed const result = route(p_response, sequence);
         if (result == routed::target) {
            return p_response;
         }
         if (
            result == routed::failed
            || (result == routed::event && !queue_event(p_response))
         ) {
            break;
         }
      }
      if (m_has_failed) {
         return __builtin_bit_cast(cat::byte const*, &m_failure);
      }
      auto _ = transfer(transfer_mode::receive);
   }
}

auto
x11::connection::next_event(transfer_mode mode) noexcept
   -> event const* _Nullable {
   release_taken();
   if (m_event_head != m_event_tail) {
      auto const* p_event =
         __builtin_bit_cast(event const*, m_p_events + m_event_head);
      m_event_head += response_size(m_p_events + m_event_head);
      if (m_event_head == m_event_tail) {
         m_event_head = 0u;
         m_event_tail = 0u;
      }
      return p_event;
   }

   for (bool has_received = false;;) {
      while (cat::byte const* p_response = next_response()) {
         if (p_response[0].value > 1u) {
            return __builtin_bit_cast(event const*, p_response);
         }
         // Replies and errors are kept for their waits, unless they belong to
         // requests without replies.
         routed const result = route(p_response, 0u);
         if (result == routed::event) {
            return __builtin_bit_cast(event const*, p_response);
         }
         if (result == routed::failed) {
            return nullptr;
         }
      }
      if (mode == transfer_mode::poll) {
         if (has_received) {
            return nullptr;
         }
         // While the watched socket has stayed unreadable and no completion
         // waits for this thread, there is nothing to receive.
         if (
            m_watch_operation.is_armed && !m_watch_operation.is_readable
            && m_send_size == 0u
            && !m_p_ring->submission_queue().has_task_work()
            && m_p_ring->completion_queue().ready_count() == 0u
         ) {
            return nullptr;
         }
         // Data that arrives after this sets it again, so none is missed.
         m_watch_operation.is_readable = false;
      }
      if (!transfer(mode)) {
         return nullptr;
      }
      has_received = true;
   }
}

auto
x11::connection::flush() noexcept -> scaredy_x11<void> {
   if (!transfer(transfer_mode::send)) {
      return m_failure;
   }
   return cat::monostate;
}

auto
x11::connection::check(cookie<void> request) noexcept -> scaredy_x11<void> {
   if (request.sequence == 0u) {
      return m_unsent;
   }
   m_check_sequence = request.sequence;
   m_has_checked = false;
   cat::byte const* p_response = wait_response(get_input_focus().sequence);
   m_check_sequence = 0u;
   if (p_response[0] == 0u) {
      return to_reply<void>(p_response);
   }
   if (m_has_checked) {
      return m_checked;
   }
   // An earlier wait may have queued its error as an event already.
   cat::uint2 const low_sequence = detail::low_sequence(request.sequence);
   for (cat::idx offset = m_event_head; offset < m_event_tail;
        offset += response_size(m_p_events + offset)) {
      error_packet failure;
      __builtin_memcpy_inline(&failure, m_p_events + offset, sizeof(failure));
      if (failure.response_type == 0u && failure.sequence == low_sequence) {
         __builtin_memmove(
            m_p_events + offset, m_p_events + offset + 32u,
            cat::idx(m_event_tail - offset - 32u)
         );
         m_event_tail = cat::idx(m_event_tail - 32u);
         return failure;
      }
   }
   return cat::monostate;
}

auto
x11::connection::poll_event() noexcept -> scaredy_x11<event const*> {
   event const* p_event = next_event(transfer_mode::poll);
   if (p_event == nullptr && m_has_failed) {
      return m_failure;
   }
   return p_event;
}

auto
x11::connection::wait_event() noexcept -> scaredy_x11<event const*> {
   event const* p_event = next_event(transfer_mode::receive);
   if (p_event == nullptr) {
      return m_failure;
   }
   return p_event;
}

auto
x11::connection::create_window(window_options const& options) noexcept
   -> window {
   window const id = generate_id();

   struct {
      request_header header;
      x11::window id, parent;
      cat::int2 x, y;
      cat::uint2 width, height, border_width;
      window_class class_type;
      x11::visual visual;
      cat::uint4 mask;
   } const request{
      .header = {operation_code::create_window, options.depth},
      .id = id,
      .parent = options.parent == window::none ? m_screen.root : options.parent,
      .x = options.x,
      .y = options.y,
      .width = options.width,
      .height = options.height,
      .border_width = options.border_width,
      .class_type = options.class_type,
      .visual = options.visual,
      .mask = options.attributes.mask
   };

   static_assert(sizeof(request) == 32);
   push_with_values<void>(request, options.attributes);
   return id;
}

auto
x11::connection::set_title(window target, cat::str_view title) noexcept
   -> cookie<void> {
   change_property(
      target, atoms::wm_name, m_utf8_string, property_format::bits_8,
      as_bytes(title)
   );
   return change_property(
      target, m_net_wm_name, m_utf8_string, property_format::bits_8,
      as_bytes(title)
   );
}

auto
x11::connection::enable_close_request(window target) noexcept -> cookie<void> {
   return change_property(
      target, m_wm_protocols, atoms::atom, property_format::bits_32,
      {__builtin_bit_cast(cat::byte const*, &m_wm_delete_window), 4u}
   );
}

auto
x11::connection::is_close_request(event const& event) const noexcept -> bool {
   if (event.code() != event_code::client_message) {
      return false;
   }
   auto const& message = event.as<client_message_event>();
   return message.type == m_wm_protocols && message.format == 32u
          && message.data_32[0] == m_wm_delete_window.value;
}
