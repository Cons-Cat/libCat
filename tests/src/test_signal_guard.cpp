#include <cat/array>
#include <cat/linux>
#include <cat/signal_guard>
#include <cat/string>

#include "../unit_tests.hpp"

#ifndef CAT_NO_SIGNAL_HANDLER

$test(signal_guard_returns_callback_result) {
   cat::scaredy_signal<int> const result =
      cat::signal_guard(cat::signal_set::segmentation_violation, [] {
         return 1;
      });

   cat::verify(result.value() == 1);
}

$test(signal_guard_returns_void) {
   bool called = false;
   cat::scaredy_signal<> const result =
      cat::signal_guard(cat::signal_set::segmentation_violation, [&] {
         called = true;
      });

   cat::verify(result.has_value());
   cat::verify(called);
}

$test(signal_guard_catches_fault) {
   int* volatile p_bad = __builtin_bit_cast(int*, 8ull);

   cat::scaredy_signal<int> const result =
      cat::signal_guard(cat::signal_set::segmentation_violation, [&] {
         // Dereference privileged memory!
         *p_bad = 0;
         return 1;
      });

   cat::verify(
      result.error<cat::signal_errors>()
      == cat::signal_errors::segmentation_violation
   );
}

$test(signal_guard_fatal_catches_abort) {
   cat::scaredy_signal<int> const result = cat::signal_guard_fatal([] {
      nix::raise_here(nix::signal::abort).verify();
      return 1;
   });

   cat::verify(result.error<cat::signal_errors>() == cat::signal_errors::abort);
}

$test(signal_guard_catches_termination_requests) {
   cat::scaredy_signal<int> const interrupt =
      cat::signal_guard(cat::signal_set::interrupt, [] {
         nix::raise_here(nix::signal::interrupt).verify();
         return 1;
      });
   cat::verify(
      interrupt.error<cat::signal_errors>() == cat::signal_errors::interrupt
   );

   cat::scaredy_signal<int> const termination =
      cat::signal_guard(cat::signal_set::graceful_termination, [] {
         nix::raise_here(nix::signal::graceful_termination).verify();
         return 1;
      });
   cat::verify(
      termination.error<cat::signal_errors>()
      == cat::signal_errors::graceful_termination
   );
}

$test(signal_guard_unites_scaredy_errors) {
   using result_type = cat::scaredy<int, nix::linux_error, cat::signal_errors>;

   // `scaredy_nix` uses compact storage, which is united through its value.
   result_type const value = cat::signal_guard_fatal([] {
      return nix::scaredy_nix<int>(1);
   });
   cat::verify(value.value() == 1);

   result_type const error = cat::signal_guard_fatal([] {
      return nix::scaredy_nix<int>(nix::linux_error::inval);
   });
   cat::verify(error.error<nix::linux_error>() == nix::linux_error::inval);

   result_type const signal = cat::signal_guard_fatal([] {
      nix::raise_here(nix::signal::abort).verify();
      return nix::scaredy_nix<int>(1);
   });
   cat::verify(signal.error<cat::signal_errors>() == cat::signal_errors::abort);

   cat::scaredy<void, nix::linux_error, cat::signal_errors> const nothing =
      cat::signal_guard_fatal([] {
         return nix::scaredy_nix<void>(cat::monostate);
      });
   cat::verify(nothing.has_value());
}

$test(signal_guard_nests) {
   cat::scaredy_signal<int> const result =
      cat::signal_guard(cat::signal_set::abort, [] {
         // The inner guard does not catch `abort`, so the outer one does.
         return cat::signal_guard(cat::signal_set::segmentation_violation, [] {
            nix::raise_here(nix::signal::abort).verify();
            return 1;
         });
      });

   cat::verify(result.error<cat::signal_errors>() == cat::signal_errors::abort);
}

namespace {

void
crash_with_fatal_signal_handler(nix::file_descriptor output) {
   nix::sys_dup2(output, nix::stderr).verify();

   // A non-null address, so UBSan does not intercept the store.
   int* volatile p_bad = __builtin_bit_cast(int*, cat::uword(8u));
   *p_bad = 0;
}

}  // namespace

$test(fatal_signal_handler) {
   cat::array<nix::file_descriptor, 2u> pipe{};
   nix::sys_pipe(pipe).verify();

   nix::process child;
   $defer {
      child.free(g_tests_alloc);
   };

   // The child needs its own signal handlers and stderr.
   child.set_clone_flags(
      static_cast<nix::clone_flags>(
         cat::to_underlying(child.get_clone_flags())
         & ~cat::to_underlying(
            nix::clone_flags::virtual_memory
            | nix::clone_flags::file_descriptor_table
         )
      )
   );

   child.spawn(g_tests_alloc, 1_umi, crash_with_fatal_signal_handler, pipe[1])
      .verify();

   cat::int4 status;
   nix::sys_wait4(child.id(), &status, nix::wait_options_flags::none, nullptr)
      .verify();
   nix::sys_close(pipe[1]).verify();

   cat::array<char, 4'096u> output{};
   idx const bytes =
      nix::sys_read(pipe[0], output.data(), output.size()).verify();
   nix::sys_close(pipe[0]).verify();
   cat::str_view const report(output.data(), bytes);

   cat::verify(
      report.find("*** Signal 11 (segmentation_violation) (0x8) ").value_or(1u)
      == 0u
   );
   cat::verify(report.find("crash_with_fatal_signal_handler").has_value());

#if !__has_feature(address_sanitizer) && !__has_feature(thread_sanitizer)
   // The default handler then kills the child with the same signal.
   cat::verify((status.raw & 0x7f) == 11);
#endif
}

#endif
