#include <cat/file>
#include <cat/linux>

namespace cat {

namespace {

template <typename T>
[[nodiscard]]
auto
expand_file_result(nix::scaredy_nix<T> result) -> scaredy<T, io_error> {
   if (result.has_value()) {
      if constexpr (is_void<T>) {
         return {};
      } else {
         return result.value();
      }
   }
   return static_cast<io_error>(-to_underlying(result.error()));
}

[[nodiscard]]
constexpr auto
has_flag(file_flags flags, file_flags flag) -> bool {
   return (flags & flag) != file_flags::none;
}

[[nodiscard]]
constexpr auto
native_disposition(file_access access, file_flags flags)
   -> native_handle_disposition {
   native_handle_disposition disposition =
      native_handle_disposition::file
      | native_handle_disposition::kernel_handle
      | native_handle_disposition::seekable;
   switch (access) {
      case file_access::read:
         disposition |= native_handle_disposition::readable;
         break;
      case file_access::write:
         disposition |= native_handle_disposition::writable;
         break;
      case file_access::append:
         disposition |= native_handle_disposition::writable
                        | native_handle_disposition::append_only;
         break;
      case file_access::read_write:
         disposition |= native_handle_disposition::readable
                        | native_handle_disposition::writable;
         break;
   }
   if (has_flag(flags, file_flags::nonblocking)) {
      disposition |= native_handle_disposition::nonblocking;
   }
   return disposition;
}

[[nodiscard]]
constexpr auto
linux_seek_origin(seek_origin origin) -> nix::seek_whence {
   switch (origin) {
      case seek_origin::beginning:
         return nix::seek_whence::beginning;
      case seek_origin::current:
         return nix::seek_whence::current;
      case seek_origin::end:
         return nix::seek_whence::end;
   }
}

}  // namespace

auto
file_handle::operator=(file_handle&& other) -> file_handle& {
   if (this == __builtin_addressof(other)) {
      return *this;
   }
   auto _ = close();
   m_handle = other.release();
   return *this;
}

file_handle::~file_handle() {
   auto _ = close();
}

auto
file_handle::open(
   file_path const& file_path, file_access access, file_creation creation,
   file_flags flags, file_permissions permissions
) -> scaredy<file_handle, io_error> {
   return open_from({}, file_path, access, creation, flags, permissions);
}

auto
file_handle::open_at(
   file_handle const& directory, file_path const& file_path, file_access access,
   file_creation creation, file_flags flags, file_permissions permissions
) -> scaredy<file_handle, io_error> {
   return open_from(
      directory.native_handle(), file_path, access, creation, flags, permissions
   );
}

auto
file_handle::close() -> scaredy<void, io_error> {
   if (!is_open()) {
      return {};
   }
   cat::native_handle const handle = release();
   iword const value = detail::native_handle_access::value(handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<void>(nix::sys_close(descriptor));
}

auto
file_handle::read(span<char> buffer) const -> scaredy<idx, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<idx>(nix::sys_read(descriptor, buffer));
}

auto
file_handle::write(str_view buffer) const -> scaredy<idx, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<idx>(nix::sys_write(descriptor, buffer));
}

auto
file_handle::read_at(span<char> buffer, iword offset) const
   -> scaredy<idx, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<idx>(nix::sys_pread64(descriptor, buffer, offset));
}

auto
file_handle::write_at(str_view buffer, iword offset) const
   -> scaredy<idx, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<idx>(
      nix::sys_pwrite64(descriptor, buffer, offset)
   );
}

auto
file_handle::seek(iword offset, seek_origin origin) const
   -> scaredy<idx, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<idx>(
      nix::sys_lseek(descriptor, offset, linux_seek_origin(origin))
   );
}

auto
file_handle::truncate(iword size) const -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<void>(nix::sys_ftruncate(descriptor, size));
}

auto
file_handle::sync() const -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<void>(nix::sys_fsync(descriptor));
}

auto
file_handle::data_sync() const -> scaredy<void, io_error> {
   iword const value = detail::native_handle_access::value(m_handle);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   return expand_file_result<void>(nix::sys_fdatasync(descriptor));
}

auto
file_handle::open_from(
   cat::native_handle directory, file_path const& file_path, file_access access,
   file_creation creation, file_flags file_options, file_permissions permissions
) -> scaredy<file_handle, io_error> {
   nix::open_mode mode;
   nix::open_flags flags = nix::open_flags::close_exec;
   switch (access) {
      case file_access::read:
         mode = nix::open_mode::read_only;
         break;
      case file_access::write:
         mode = nix::open_mode::write_only;
         break;
      case file_access::append:
         mode = nix::open_mode::write_only;
         flags |= nix::open_flags::append_file;
         break;
      case file_access::read_write:
         mode = nix::open_mode::read_write;
         break;
   }

   switch (creation) {
      case file_creation::open_existing:
         break;
      case file_creation::create_new:
         flags |= nix::open_flags::create | nix::open_flags::exclusive;
         break;
      case file_creation::open_or_create:
         flags |= nix::open_flags::create;
         break;
      case file_creation::truncate_existing:
         flags |= nix::open_flags::truncate;
         break;
   }

   if (has_flag(file_options, file_flags::nonblocking)) {
      flags |= nix::open_flags::nonblocking;
   }
   if (has_flag(file_options, file_flags::data_sync)) {
      flags |= nix::open_flags::dsync;
   }
   if (has_flag(file_options, file_flags::direct)) {
      flags |= nix::open_flags::direct;
   }
   if (has_flag(file_options, file_flags::directory)) {
      flags |= nix::open_flags::directory;
   }
   if (has_flag(file_options, file_flags::no_follow)) {
      flags |= nix::open_flags::nofollow;
   }
   if (has_flag(file_options, file_flags::no_access_time)) {
      flags |= nix::open_flags::noatime;
   }
   if (has_flag(file_options, file_flags::sync)) {
      flags |= nix::open_flags::sync;
   }

   nix::file_descriptor directory_descriptor = nix::at_fdcwd;
   if (directory.is_valid()) {
      iword const value = detail::native_handle_access::value(directory);
      directory_descriptor = value == -1 ? nix::invalid_file_descriptor
                                         : nix::file_descriptor(uint4(value));
   }
   nix::scaredy_nix<nix::file_descriptor> result = nix::sys_openat(
      directory_descriptor, file_path.native(), mode, flags,
      static_cast<nix::file_permissions>(permissions)
   );
   if (result.has_value()) {
      nix::file_descriptor const descriptor = result.value();
      if (descriptor.value == nix::invalid_file_descriptor.value) {
         return adopt({});
      }
      return adopt(
         detail::native_handle_access::make(
            descriptor.value, native_disposition(access, file_options)
                                 | native_handle_disposition::kernel_handle
         )
      );
   }
   return static_cast<io_error>(-to_underlying(result.error()));
}

}  // namespace cat
