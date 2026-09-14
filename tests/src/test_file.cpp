#include <cat/file>
#include <cat/inode>
#include <cat/linux>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

$test(file_executable_path) {
   cat::file_path executable = cat::get_executable_path(pager).verify();
   $defer {
      executable.free(pager);
   };

   cat::verify(!executable.is_empty());
   cat::verify(executable.is_absolute());
   cat::verify(executable.data()[executable.size()] == '\0');
   cat::verify(executable.filename() == "unit_tests");
   nix::sys_access(executable.native(), nix::access_mode::exists).verify();
}

$test(file_handle_lifecycle) {
   auto file_path =
      cat::make_file_path_unique(pager, "/tmp/libcat-file-lifecycle-%%%%")
         .verify();
   $defer {
      auto _ = nix::sys_unlink(file_path.native());
      file_path.free(pager);
   };

   auto handle =
      cat::file_handle::open(
         file_path, cat::file_access::read_write, cat::file_creation::create_new
      )
         .verify();
   cat::verify(handle.is_open());

   cat::native_handle const native = handle.native_handle();
   cat::verify(native.is_valid());
   cat::verify(native.is_kernel_handle());
   cat::verify(native.is_file());
   cat::verify(native.is_readable());
   cat::verify(native.is_writable());
   cat::verify(native.is_seekable());
   cat::file_handle moved(cat::move(handle));
   cat::verify(!handle.is_open());
   cat::verify(moved.native_handle() == native);

   cat::file_handle assigned;
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

   auto adopted = cat::file_handle::adopt(cat::move(released));
   cat::verify(!released.is_valid());
   adopted.close().verify();
   cat::verify(!adopted.is_open());
   adopted.close().verify();
}

$test(file_handle_io_and_metadata) {
   auto file_path =
      cat::make_file_path_unique(pager, "/tmp/libcat-file-io-%%%%").verify();
   $defer {
      auto _ = nix::sys_unlink(file_path.native());
      file_path.free(pager);
   };

   auto handle =
      cat::file_handle::open(
         file_path, cat::file_access::read_write, cat::file_creation::create_new
      )
         .verify();

   cat::verify(handle.write("abcdef").verify() == 6u);
   cat::verify(handle.seek(0, cat::seek_origin::beginning).verify() == 0u);

   char sequential[6] = {};
   cat::verify(handle.read(sequential).verify() == 6u);
   cat::verify(sequential[0] == 'a');
   cat::verify(sequential[5] == 'f');

   cat::verify(handle.write_at("XY", 2).verify() == 2u);
   cat::verify(handle.seek(0, cat::seek_origin::current).verify() == 6u);

   char positioned[6] = {};
   cat::verify(handle.read_at(positioned, 0).verify() == 6u);
   cat::verify(positioned[0] == 'a');
   cat::verify(positioned[1] == 'b');
   cat::verify(positioned[2] == 'X');
   cat::verify(positioned[3] == 'Y');
   cat::verify(positioned[4] == 'e');
   cat::verify(positioned[5] == 'f');

   handle.truncate(4).verify();
   handle.sync().verify();
   handle.data_sync().verify();

   nix::inode const metadata =
      nix::inode_status(handle.native_handle()).verify();
   cat::verify(metadata.type == nix::file_type::regular);
   cat::verify(metadata.size == 4u);
   cat::verify(metadata.number > 0u);
   cat::verify(metadata.hard_link_count >= 1u);
   cat::verify(metadata.io_block_size > 0u);
   cat::verify(metadata.user.value == nix::sys_getuid().value);
   cat::verify(metadata.group.value == nix::sys_getgid().value);
   cat::verify(
      (metadata.validity & nix::inode_validity::size)
      != nix::inode_validity::none
   );

   handle.close().verify();
   auto truncated = cat::file_handle::open(
                       file_path, cat::file_access::read_write,
                       cat::file_creation::truncate_existing
   )
                       .verify();
   cat::verify(
      nix::inode_status(truncated.native_handle()).verify().size == 0u
   );
}

$test(file_handle_anchored_open) {
   auto directory_path =
      cat::make_file_path_unique(pager, "/tmp/libcat-file-directory-%%%%")
         .verify();
   auto child_path = cat::make_file_path(pager, "child").verify();
   $defer {
      auto _ = nix::sys_rmdir(directory_path.native());
      child_path.free(pager);
      directory_path.free(pager);
   };

   nix::sys_mkdir(
      directory_path.native(), nix::file_permissions::user_read
                                  | nix::file_permissions::user_write
                                  | nix::file_permissions::user_execute
   )
      .verify();

   auto directory =
      cat::file_handle::open(
         directory_path, cat::file_access::read,
         cat::file_creation::open_existing, cat::file_flags::directory
      )
         .verify();
   auto child = cat::file_handle::open_at(
                   directory, child_path, cat::file_access::read_write,
                   cat::file_creation::create_new
   )
                   .verify();
   cat::verify(child.write("anchored").verify() == 8u);
   cat::verify(nix::inode_status(child.native_handle()).verify().size == 8u);
   child.close().verify();
   cat::iword const directory_value =
      cat::detail::native_handle_access::value(directory.native_handle());
   nix::sys_unlinkat(
      directory_value == -1 ? nix::invalid_file_descriptor
                            : nix::file_descriptor(cat::uint4(directory_value)),
      child_path.native(), nix::atfile_flags::none
   )
      .verify();
   directory.close().verify();
}

$test(mapped_file_handle_read_only) {
   auto file_path =
      cat::make_file_path_unique(pager, "/tmp/libcat-mapped-read-%%%%")
         .verify();
   $defer {
      auto _ = nix::sys_unlink(file_path.native());
      file_path.free(pager);
   };

   auto file =
      cat::file_handle::open(
         file_path, cat::file_access::read_write, cat::file_creation::create_new
      )
         .verify();
   cat::verify(file.write("mapped data").verify() == 11u);
   file.close().verify();

   auto mapped = cat::mapped_file_handle::open(file_path).verify();
   cat::verify(mapped.is_open());
   cat::verify(mapped.is_mapped());
   cat::verify(mapped.mapping_length() == 11u);
   cat::verify(mapped.address() != nullptr);
   cat::verify(mapped.view().size() == 11u);
   cat::verify(mapped.view()[0] == 'm');
   cat::verify(mapped.view()[10] == 'a');
   cat::verify(!mapped.mutable_view().has_value());
   cat::verify(
      mapped.mutable_view().error() == cat::io_error::permission_denied
   );

   cat::native_handle const native = mapped.native_handle();
   cat::verify(
      native.disposition()
      == (cat::native_handle_disposition::file | cat::native_handle_disposition::kernel_handle | cat::native_handle_disposition::seekable | cat::native_handle_disposition::readable)
   );

   mapped.close().verify();
   cat::verify(!mapped.is_open());
   cat::verify(!mapped.is_mapped());
   cat::verify(mapped.mapping_length() == 0u);
   mapped.close().verify();
}

$test(mapped_file_handle_shared_write_and_move) {
   auto file_path =
      cat::make_file_path_unique(pager, "/tmp/libcat-mapped-write-%%%%")
         .verify();
   $defer {
      auto _ = nix::sys_unlink(file_path.native());
      file_path.free(pager);
   };

   auto file =
      cat::file_handle::open(
         file_path, cat::file_access::read_write, cat::file_creation::create_new
      )
         .verify();
   cat::verify(file.write("abcdef").verify() == 6u);

   auto mapped = cat::mapped_file_handle::map(cat::move(file)).verify();
   cat::verify(!file.is_open());
   cat::verify(mapped.mutable_address().verify() == mapped.address());
   auto bytes = mapped.mutable_view().verify();
   bytes[2] = cat::byte('X');
   bytes[3] = cat::byte('Y');

   char positioned[6] = {};
   auto observer = cat::file_handle::open(file_path).verify();
   cat::verify(observer.read_at(positioned, 0).verify() == 6u);
   cat::verify(positioned[2] == 'X');
   cat::verify(positioned[3] == 'Y');
   observer.close().verify();

   cat::native_handle const native = mapped.native_handle();
   cat::mapped_file_handle moved(cat::move(mapped));
   cat::verify(!mapped.is_open());
   cat::verify(!mapped.is_mapped());
   cat::verify(moved.native_handle() == native);

   cat::mapped_file_handle assigned;
   assigned = cat::move(moved);
   cat::verify(!moved.is_open());
   cat::verify(!moved.is_mapped());
   cat::verify(assigned.native_handle() == native);

   cat::native_handle released = assigned.release().verify();
   cat::verify(!assigned.is_open());
   cat::verify(!assigned.is_mapped());
   cat::verify(released == native);

   auto adopted = cat::file_handle::adopt(cat::move(released));
   auto remapped = cat::mapped_file_handle::map(cat::move(adopted)).verify();
   cat::verify(!adopted.is_open());
   cat::verify(remapped.is_mapped());
   remapped.close().verify();
}

$test(mapped_file_handle_empty_and_append) {
   auto file_path =
      cat::make_file_path_unique(pager, "/tmp/libcat-mapped-empty-%%%%")
         .verify();
   $defer {
      auto _ = nix::sys_unlink(file_path.native());
      file_path.free(pager);
   };

   auto mapped =
      cat::mapped_file_handle::open(
         file_path, cat::file_access::read_write, cat::file_creation::create_new
      )
         .verify();
   cat::verify(mapped.is_open());
   cat::verify(!mapped.is_mapped());
   cat::verify(mapped.address() == nullptr);
   cat::verify(mapped.mapping_length() == 0u);
   cat::verify(mapped.view().size() == 0u);
   cat::verify(mapped.mutable_view().verify().size() == 0u);
   mapped.close().verify();

   auto append =
      cat::file_handle::open(file_path, cat::file_access::append).verify();
   auto rejected = cat::mapped_file_handle::map(cat::move(append));
   cat::verify(!rejected.has_value());
   cat::verify(rejected.error() == cat::io_error::invalid_argument);
   cat::verify(append.is_open());
   append.close().verify();

   auto open_rejected =
      cat::mapped_file_handle::open(file_path, cat::file_access::append);
   cat::verify(!open_rejected.has_value());
   cat::verify(open_rejected.error() == cat::io_error::invalid_argument);
}
