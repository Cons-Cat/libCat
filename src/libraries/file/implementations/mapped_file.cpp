#include <cat/file>
#include <cat/linux>

namespace cat {

namespace {

[[nodiscard]]
constexpr auto
is_mapping_access(file_access access) -> bool {
   return access == file_access::read || access == file_access::read_write;
}

}  // namespace

auto
mapped_file_handle::operator=(mapped_file_handle&& other)
   -> mapped_file_handle& {
   if (this == __builtin_addressof(other)) {
      return *this;
   }
   auto _ = close();
   m_file = move(other).m_file;
   m_p_mapping = other.m_p_mapping;
   m_mapping_length = other.m_mapping_length;
   other.m_p_mapping = nullptr;
   other.m_mapping_length = 0u;
   return *this;
}

mapped_file_handle::~mapped_file_handle() {
   auto _ = close();
}

auto
mapped_file_handle::open(
   file_path const& file_path, file_access access, file_creation creation,
   file_flags flags, file_permissions permissions
) -> scaredy<mapped_file_handle, io_error> {
   if (!is_mapping_access(access)) {
      return io_error::invalid_argument;
   }
   scaredy<file_handle, io_error> file =
      file_handle::open(file_path, access, creation, flags, permissions);
   if (!file.has_value()) {
      return file.error();
   }
   return map(move(file.value()));
}

auto
mapped_file_handle::open_at(
   file_handle const& directory, file_path const& file_path, file_access access,
   file_creation creation, file_flags flags, file_permissions permissions
) -> scaredy<mapped_file_handle, io_error> {
   if (!is_mapping_access(access)) {
      return io_error::invalid_argument;
   }
   scaredy<file_handle, io_error> file = file_handle::open_at(
      directory, file_path, access, creation, flags, permissions
   );
   if (!file.has_value()) {
      return file.error();
   }
   return map(move(file.value()));
}

auto
mapped_file_handle::map(file_handle&& file)
   -> scaredy<mapped_file_handle, io_error> {
   cat::native_handle const native = file.native_handle();
   if (!native.is_valid()) {
      return io_error::bad_file_descriptor;
   }
   if (native.is_append_only() || !native.is_readable()) {
      return io_error::invalid_argument;
   }

   iword const value = detail::native_handle_access::value(native);
   nix::file_descriptor const descriptor =
      value == -1 ? nix::invalid_file_descriptor
                  : nix::file_descriptor(uint4(value));
   scaredy<nix::file_status, nix::linux_error> status =
      nix::sys_fstat(descriptor);
   if (!status.has_value()) {
      return static_cast<io_error>(-to_underlying(status.error()));
   }
   idx const mapping_length = status.value().file_size;
   if (mapping_length == 0u) {
      return mapped_file_handle(move(file), nullptr, 0u);
   }

   nix::memory_protection_flags const protections =
      native.is_writable() ? nix::memory_protection_flags::read_write
                           : nix::memory_protection_flags::read;
   nix::scaredy_nix<byte*> mapping = nix::sys_mmap(
      nullptr, mapping_length, protections, nix::memory_flags::shared,
      descriptor, 0u
   );
   if (!mapping.has_value()) {
      return static_cast<io_error>(-to_underlying(mapping.error()));
   }
   return mapped_file_handle(move(file), mapping.value(), mapping_length);
}

auto
mapped_file_handle::unmap() -> scaredy<void, io_error> {
   if (!is_mapped()) {
      return {};
   }
   byte const* const p_mapping = m_p_mapping;
   idx const mapping_length = m_mapping_length;
   m_p_mapping = nullptr;
   m_mapping_length = 0u;
   nix::scaredy_nix<void> const result =
      nix::sys_munmap(p_mapping, mapping_length);
   if (!result.has_value()) {
      return static_cast<io_error>(-to_underlying(result.error()));
   }
   return {};
}

auto
mapped_file_handle::release() -> scaredy<cat::native_handle, io_error> {
   scaredy<void, io_error> const result = unmap();
   if (!result.has_value()) {
      return result.error();
   }
   return m_file.release();
}

auto
mapped_file_handle::close() -> scaredy<void, io_error> {
   scaredy<void, io_error> const mapping_result = unmap();
   scaredy<void, io_error> const file_result = m_file.close();
   if (!mapping_result.has_value()) {
      return mapping_result.error();
   }
   if (!file_result.has_value()) {
      return file_result.error();
   }
   return {};
}

}  // namespace cat
