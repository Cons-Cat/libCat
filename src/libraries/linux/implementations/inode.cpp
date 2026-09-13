#include <cat/inode>

namespace {

[[nodiscard]]
auto
inode_type(cat::uint4 mode) -> nix::file_type {
   cat::uint4 const type = mode & 0170000u;
   if (type == 0010000u) {
      return nix::file_type::fifo;
   }
   if (type == 0020000u) {
      return nix::file_type::character_device;
   }
   if (type == 0040000u) {
      return nix::file_type::directory;
   }
   if (type == 0060000u) {
      return nix::file_type::block_device;
   }
   if (type == 0100000u) {
      return nix::file_type::regular;
   }
   if (type == 0120000u) {
      return nix::file_type::symbolic_link;
   }
   if (type == 0140000u) {
      return nix::file_type::socket;
   }
   return nix::file_type::unknown;
}

[[nodiscard]]
auto
device_major(cat::uint8 device) -> cat::uint4 {
   return cat::uint4(
      ((device >> 8u) & 0xfffu) | ((device >> 32u) & 0xfffff000u)
   );
}

[[nodiscard]]
auto
device_minor(cat::uint8 device) -> cat::uint4 {
   return cat::uint4((device & 0xffu) | ((device >> 12u) & 0xffffff00u));
}

[[nodiscard]]
auto
inode_timestamp(nix::statx_timestamp const& timestamp) -> nix::inode_timestamp {
   return {
      .seconds = timestamp.seconds,
      .nanoseconds = timestamp.nanoseconds,
   };
}

[[nodiscard]]
auto
inode_timestamp(nix::file_status::time_spec const& timestamp)
   -> nix::inode_timestamp {
   return {
      .seconds = timestamp.seconds,
      .nanoseconds = cat::uint4(timestamp.nanoseconds),
   };
}

[[nodiscard]]
auto
empty_inode_timestamp() -> nix::inode_timestamp {
   return {};
}

[[nodiscard]]
constexpr auto
has_statx_field(nix::statx_mask mask, nix::statx_mask field) -> bool {
   return (mask & field) == field;
}

[[nodiscard]]
auto
inode_validity(nix::statx_mask mask) -> nix::inode_validity {
   nix::inode_validity result =
      nix::inode_validity::device | nix::inode_validity::io_block_size;
   if (has_statx_field(mask, nix::statx_mask::type)) {
      result |= nix::inode_validity::type;
   }
   if (has_statx_field(mask, nix::statx_mask::mode)) {
      result |= nix::inode_validity::permissions;
   }
   if (has_statx_field(mask, nix::statx_mask::nlink)) {
      result |= nix::inode_validity::hard_link_count;
   }
   if (has_statx_field(mask, nix::statx_mask::uid)) {
      result |= nix::inode_validity::user;
   }
   if (has_statx_field(mask, nix::statx_mask::gid)) {
      result |= nix::inode_validity::group;
   }
   if (has_statx_field(mask, nix::statx_mask::atime)) {
      result |= nix::inode_validity::access_time;
   }
   if (has_statx_field(mask, nix::statx_mask::mtime)) {
      result |= nix::inode_validity::modification_time;
   }
   if (has_statx_field(mask, nix::statx_mask::ctime)) {
      result |= nix::inode_validity::change_time;
   }
   if (has_statx_field(mask, nix::statx_mask::ino)) {
      result |= nix::inode_validity::number;
   }
   if (has_statx_field(mask, nix::statx_mask::size)) {
      result |= nix::inode_validity::size;
   }
   if (has_statx_field(mask, nix::statx_mask::blocks)) {
      result |= nix::inode_validity::allocated_size;
   }
   if (has_statx_field(mask, nix::statx_mask::btime)) {
      result |= nix::inode_validity::birth_time;
   }
   return result;
}

[[nodiscard]]
auto
normalize(nix::statx_data const& status) -> nix::inode {
   cat::uint4 const mode = status.protections_mode;
   return {
      .validity = inode_validity(static_cast<nix::statx_mask>(status.mask)),
      .type = inode_type(mode),
      .permissions = static_cast<nix::file_permissions>(mode & 0007777u),
      .device_major = status.device_major,
      .device_minor = status.device_minor,
      .number = status.inode,
      .user = {status.user},
      .group = {status.group},
      .hard_link_count = status.hard_link_count,
      .size = cat::idx(status.file_size),
      .allocated_size = cat::idx(status.blocks_count * 512u),
      .io_block_size = status.block_size,
      .access_time = inode_timestamp(status.last_access_time),
      .modification_time = inode_timestamp(status.last_modification_time),
      .change_time = inode_timestamp(status.last_change_time),
      .birth_time = inode_timestamp(status.creation_time),
   };
}

[[nodiscard]]
auto
normalize(nix::file_status const& status) -> nix::inode {
   return {
      .validity = nix::inode_validity::type
                  | nix::inode_validity::permissions
                  | nix::inode_validity::hard_link_count
                  | nix::inode_validity::user
                  | nix::inode_validity::group
                  | nix::inode_validity::access_time
                  | nix::inode_validity::modification_time
                  | nix::inode_validity::change_time
                  | nix::inode_validity::number
                  | nix::inode_validity::size
                  | nix::inode_validity::allocated_size
                  | nix::inode_validity::device
                  | nix::inode_validity::io_block_size,
      .type = inode_type(status.protections_mode),
      .permissions =
         static_cast<nix::file_permissions>(status.protections_mode & 0007777u),
      .device_major = device_major(status.device_id),
      .device_minor = device_minor(status.device_id),
      .number = status.inode,
      .user = status.user,
      .group = status.groud,
      .hard_link_count = status.hard_links_count,
      .size = status.file_size,
      .allocated_size = status.blocks_count * 512u,
      .io_block_size = status.block_size,
      .access_time = inode_timestamp(status.last_access_time),
      .modification_time = inode_timestamp(status.last_modification_time),
      .change_time = inode_timestamp(status.status_change_time),
      .birth_time = empty_inode_timestamp(),
   };
}

}  // namespace

auto
nix::inode_status(file_descriptor descriptor)
   -> cat::scaredy<inode, linux_error> {
   statx_data extended_status{};
   scaredy_nix<void> result = sys_statx(
      descriptor, "", atfile_flags::empty_path | atfile_flags::no_auto_mount,
      statx_mask::basic_stats | statx_mask::btime, extended_status
   );
   if (result.has_value()) {
      return normalize(extended_status);
   }
   if (
      result.error() != linux_error::nosys
      && result.error() != linux_error::inval
   ) {
      return result.error();
   }

   cat::scaredy<file_status, linux_error> fallback = sys_fstat(descriptor);
   if (fallback.has_value()) {
      return normalize(fallback.value());
   }
   return fallback.error();
}

auto
nix::inode_status(cat::native_handle handle)
   -> cat::scaredy<inode, linux_error> {
   cat::iword const value = cat::detail::native_handle_access::value(handle);
   return inode_status(
      value == -1 ? invalid_file_descriptor : file_descriptor(cat::uint4(value))
   );
}
