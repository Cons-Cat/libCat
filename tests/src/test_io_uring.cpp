#include <cat/linux>

#include "../unit_tests.hpp"

$test(syscall_io_uring) {
   if (!nix::has_sys_io_uring()) {
      return;
   }

   // Avoid creating an SQ thread that would need cleanup.
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::ring_disabled;
   auto setup_result = nix::sys_io_uring_setup(4u, params);
   if (setup_result.is_empty()) {
      // Sandboxes can deny setup after a successful probe.
      cat::verify(
         setup_result.error() == nix::linux_error::perm
         || setup_result.error() == nix::linux_error::nosys
      );
      return;
   }
   nix::file_descriptor ring = setup_result.value();

   auto enter_result =
      nix::sys_io_uring_enter(ring, 0u, 0u, nix::io_uring_enter_flags::none);
   cat::verify(
      enter_result.has_value()
      || enter_result.error() == nix::linux_error::badfd
   );

   nix::sys_close(ring).verify();
}
