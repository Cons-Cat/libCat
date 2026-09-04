#include <cat/io_uring>

#include "../unit_tests.hpp"

// NOLINTBEGIN(readability-magic-numbers)

namespace {

[[nodiscard]]
auto
make_ring(cat::uint4 entries = 8u, nix::io_uring_params params = {})
   -> cat::scaredy<nix::io_uring, nix::linux_error> {
   auto made = nix::make_io_uring(entries, params);
   if (made.is_empty()) {
      cat::verify(
         made.error() == nix::linux_error::perm
         || made.error() == nix::linux_error::nosys
         || made.error() == nix::linux_error::opnotsupp
      );
   }
   return made;
}

[[nodiscard]]
auto
make_and_close_ring() -> nix::scaredy_nix<nix::file_descriptor> {
   nix::io_uring ring =
      $prop(nix::make_io_uring(4u));  // NOLINT(misc-const-correctness)
   nix::file_descriptor const descriptor = ring.descriptor();
   $defer {
      ring.close().verify();
   };
   return descriptor;
}

void
verify_empty(nix::io_uring const& ring) {
   cat::verify(ring.descriptor().value == nix::invalid_file_descriptor.value);
   cat::verify(ring.params().sq_entries == 0u);
   cat::verify(ring.params().cq_entries == 0u);
   cat::verify(ring.params().flags == nix::io_uring_setup_flags::none);
   cat::verify(ring.params().features == nix::io_uring_features::none);
}

void
verify_setup(nix::io_uring const& ring, nix::io_uring_setup_flags flags) {
   cat::verify(ring.descriptor().value != nix::invalid_file_descriptor.value);
   cat::verify(ring.params().sq_entries != 0u);
   cat::verify(ring.params().cq_entries != 0u);
   cat::verify(ring.params().flags == flags);
   cat::verify(
      (ring.params().features & nix::io_uring_features::single_mmap)
      != nix::io_uring_features::none
   );
}

void
verify_open(nix::file_descriptor descriptor) {
   cat::verify(
      nix::sys_fcntl(descriptor, nix::fcntl_command::get_fd_flags).has_value()
   );
}

void
verify_closed(nix::file_descriptor descriptor) {
   auto result = nix::sys_fcntl(descriptor, nix::fcntl_command::get_fd_flags);
   cat::verify(result.is_empty());
   cat::verify(result.error() == nix::linux_error::badf);
}

void
queue_nop(nix::io_uring& ring, cat::uint8 user_data) {
   nix::io_uring_sqe* p_submission = ring.submission_queue().get_sqe();
   cat::verify(p_submission != nullptr);
   p_submission->opcode = nix::io_uring_opcode::nop;
   p_submission->user_data = user_data;
}

void
verify_nop_completion(nix::io_uring& ring, cat::uint8 user_data) {
   nix::io_uring_cqe const* p_completion = ring.completion_queue().peek();
   cat::verify(p_completion != nullptr);
   cat::verify(p_completion->user_data == user_data);
   cat::verify(p_completion->result == 0);
   ring.completion_queue().advance();
}

void
complete_nop(nix::io_uring& ring, cat::uint8 user_data) {
   cat::verify(ring.submit_and_wait(1u).verify() == 1u);
   verify_nop_completion(ring, user_data);
}

}  // namespace

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
   nix::file_descriptor const ring = setup_result.value();

   auto enter_result =
      nix::sys_io_uring_enter(ring, 0u, 0u, nix::io_uring_enter_flags::none);
   cat::verify(
      enter_result.has_value()
      || enter_result.error() == nix::linux_error::badfd
   );

   nix::sys_close(ring).verify();
}

$test(io_uring_factory_rejects_no_mmap) {
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::no_mmap;

   auto made = nix::make_io_uring(4u, params);
   cat::verify(made.is_empty());
   cat::verify(made.error() == nix::linux_error::inval);
}

$test(io_uring_factory_prop_defer) {
   auto made = make_and_close_ring();
   if (made.is_empty()) {
      cat::verify(
         made.error() == nix::linux_error::perm
         || made.error() == nix::linux_error::nosys
         || made.error() == nix::linux_error::opnotsupp
      );
      return;
   }
   verify_closed(made.value());
}

$test(io_uring_factory_and_queues) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();

   verify_setup(ring, nix::io_uring_setup_flags::none);
   verify_open(ring.descriptor());
   cat::verify(&ring.submission_queue() == &ring.submission_queue());
   cat::verify(&ring.completion_queue() == &ring.completion_queue());

   nix::io_uring_submission_queue& submission_queue = ring.submission_queue();
   nix::io_uring_completion_queue& completion_queue = ring.completion_queue();
   cat::uint4 const entries = ring.params().sq_entries;

   cat::verify(submission_queue.ready_count() == 0u);
   cat::verify(submission_queue.space_left() == entries);
   cat::verify(submission_queue.flush() == 0u);
   cat::verify(!submission_queue.needs_wakeup());
   cat::verify(submission_queue.dropped_count() == 0u);
   cat::verify(submission_queue.get_sqe_128() == nullptr);
   cat::verify(completion_queue.peek() == nullptr);
   cat::verify(completion_queue.ready_count() == 0u);
   cat::verify(completion_queue.overflow() == 0u);

   for (cat::idx index = 0u; index < entries; ++index) {
      nix::io_uring_sqe* p_submission = submission_queue.get_sqe();
      cat::verify(p_submission != nullptr);
      cat::verify(p_submission->user_data == 0u);
      p_submission->opcode = nix::io_uring_opcode::nop;
      p_submission->user_data = 200u;
      cat::verify(submission_queue.ready_count() == index + 1u);
      cat::verify(submission_queue.space_left() == entries - index - 1u);
   }
   cat::verify(submission_queue.get_sqe() == nullptr);
   cat::verify(submission_queue.get_sqe_128() == nullptr);

   cat::uint4 const flushed = submission_queue.flush();
   cat::verify(flushed == entries);
   cat::verify(submission_queue.ready_count() == 0u);
   cat::verify(submission_queue.space_left() == 0u);
   cat::verify(submission_queue.flush() == 0u);
   cat::verify(
      ring.enter(flushed, 0u, nix::io_uring_enter_flags::none).verify()
      == flushed
   );
   cat::verify(ring.submit_and_wait(entries).verify() == 0u);
   cat::verify(completion_queue.ready_count() == entries);

   for (cat::idx index = 0u; index < entries; ++index) {
      verify_nop_completion(ring, 200u);
      cat::verify(completion_queue.ready_count() == entries - index - 1u);
   }
   cat::verify(completion_queue.peek() == nullptr);
   cat::verify(completion_queue.overflow() == 0u);
   cat::verify(submission_queue.dropped_count() == 0u);
   cat::verify(submission_queue.space_left() == entries);

   ring.close().verify();
   verify_empty(ring);
}

$test(io_uring_submit_paths) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();

   cat::verify(ring.submit().verify() == 0u);

   queue_nop(ring, 301u);
   cat::verify(ring.submit().verify() == 1u);
   cat::verify(ring.submit_and_wait(1u).verify() == 0u);
   verify_nop_completion(ring, 301u);

   queue_nop(ring, 302u);
   cat::verify(
      ring.submit(nix::io_uring_enter_flags::get_events).verify() == 1u
   );
   cat::verify(ring.submit_and_wait(1u).verify() == 0u);
   verify_nop_completion(ring, 302u);

   queue_nop(ring, 303u);
   complete_nop(ring, 303u);
   cat::verify(ring.completion_queue().peek() == nullptr);
}

$test(io_uring_extended_entries) {
   nix::io_uring_params params{};
   params.flags =
      nix::io_uring_setup_flags::sqe_128 | nix::io_uring_setup_flags::cqe_32;
   auto made = make_ring(4u, params);
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();

   verify_setup(ring, params.flags);
   nix::io_uring_submission_queue& submission_queue = ring.submission_queue();
   nix::io_uring_completion_queue& completion_queue = ring.completion_queue();

   nix::io_uring_sqe* p_extended = submission_queue.get_sqe_128();
   cat::verify(p_extended != nullptr);
   cat::verify(p_extended->user_data == 0u);
   p_extended->opcode = nix::io_uring_opcode::nop;
   p_extended->user_data = 401u;

   nix::io_uring_sqe* p_regular = submission_queue.get_sqe();
   cat::verify(p_regular != nullptr);
   cat::verify(p_regular->user_data == 0u);
   p_regular->opcode = nix::io_uring_opcode::nop;
   p_regular->user_data = 402u;

   cat::verify(submission_queue.ready_count() == 2u);
   cat::verify(ring.submit_and_wait(2u).verify() == 2u);
   cat::verify(completion_queue.ready_count() == 2u);
   verify_nop_completion(ring, 401u);
   verify_nop_completion(ring, 402u);
   cat::verify(completion_queue.peek() == nullptr);
}

$test(io_uring_no_sq_array) {
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::no_sq_array;
   auto made = make_ring(4u, params);
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();

   verify_setup(ring, params.flags);
   cat::verify(ring.submission_queue().ready_count() == 0u);
   queue_nop(ring, 450u);
   cat::verify(ring.submission_queue().ready_count() == 1u);
   complete_nop(ring, 450u);
   cat::verify(ring.submission_queue().space_left() == 4u);
}

$test(io_uring_completion_overflow) {
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::completion_size;
   params.cq_entries = 4u;
   auto made = make_ring(2u, params);
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();
   cat::verify(
      (ring.params().features & nix::io_uring_features::no_drop)
      != nix::io_uring_features::none
   );

   queue_nop(ring, 501u);
   queue_nop(ring, 501u);
   cat::verify(ring.submit().verify() == 2u);

   queue_nop(ring, 502u);
   queue_nop(ring, 502u);
   cat::verify(ring.submit_and_wait(4u).verify() == 2u);
   cat::verify(ring.completion_queue().ready_count() == 4u);

   queue_nop(ring, 503u);
   queue_nop(ring, 503u);
   cat::verify(ring.submit().verify() == 2u);

   cat::verify(ring.completion_queue().overflow() == 0u);

   verify_nop_completion(ring, 501u);
   verify_nop_completion(ring, 501u);
   verify_nop_completion(ring, 502u);
   verify_nop_completion(ring, 502u);
   cat::verify(ring.submit_and_wait(2u).verify() == 0u);
   verify_nop_completion(ring, 503u);
   verify_nop_completion(ring, 503u);
   cat::verify(ring.completion_queue().peek() == nullptr);
}

$test(io_uring_sq_poll_wakeup) {
   nix::io_uring_params params{};
   params.flags = nix::io_uring_setup_flags::sq_poll;
   params.sq_thread_idle = 1u;
   auto made = make_ring(4u, params);
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();

   for (cat::idx attempt = 0u;
        attempt < 100'000u && !ring.submission_queue().needs_wakeup();
        ++attempt) {
      nix::sys_sched_yield().verify();
   }
   cat::verify(ring.submission_queue().needs_wakeup());

   queue_nop(ring, 601u);
   cat::verify(ring.submit().verify() == 1u);
   cat::verify(ring.submit_and_wait(1u).verify() == 0u);
   verify_nop_completion(ring, 601u);
}

$test(io_uring_factory_independent_owners) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();

   nix::file_descriptor const first_descriptor = ring.descriptor();
   queue_nop(ring, 701u);

   nix::io_uring_params rejected_params{};
   rejected_params.flags = nix::io_uring_setup_flags::no_mmap;
   auto rejected = nix::make_io_uring(4u, rejected_params);
   cat::verify(rejected.is_empty());
   cat::verify(rejected.error() == nix::linux_error::inval);
   cat::verify(ring.descriptor().value == first_descriptor.value);
   complete_nop(ring, 701u);

   auto second_made = make_ring(4u);
   if (second_made.is_empty()) {
      return;
   }
   nix::io_uring second = cat::move(second_made).value();
   nix::file_descriptor const second_descriptor = second.descriptor();
   cat::verify(second_descriptor.value != first_descriptor.value);
   ring.close().verify();
   verify_closed(first_descriptor);
   verify_open(second_descriptor);
   verify_setup(second, nix::io_uring_setup_flags::none);
   queue_nop(second, 702u);
   complete_nop(second, 702u);
}

$test(io_uring_close_and_destruct) {
   nix::file_descriptor descriptor;
   {
      auto made = make_ring();
      if (made.is_empty()) {
         return;
      }
      nix::io_uring const ring = cat::move(made).value();

      descriptor = ring.descriptor();
      verify_open(descriptor);
   }
   verify_closed(descriptor);

   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();
   descriptor = ring.descriptor();
   ring.close().verify();
   verify_empty(ring);
   verify_closed(descriptor);
   ring.close().verify();
}

$test(io_uring_move_construct) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::io_uring source = cat::move(made).value();

   nix::file_descriptor const descriptor = source.descriptor();
   queue_nop(source, 101u);

   nix::io_uring moved = cat::move(source);
   verify_empty(source);  // NOLINT(bugprone-use-after-move)
   source.close().verify();
   verify_open(descriptor);

   complete_nop(moved, 101u);
   moved.close().verify();
   verify_closed(descriptor);
   moved.close().verify();
}

$test(io_uring_move_assign) {
   auto source_made = make_ring();
   if (source_made.is_empty()) {
      return;
   }
   nix::io_uring source = cat::move(source_made).value();
   auto destination_made = make_ring();
   if (destination_made.is_empty()) {
      return;
   }
   nix::io_uring destination = cat::move(destination_made).value();

   nix::file_descriptor const source_descriptor = source.descriptor();
   nix::file_descriptor const previous_descriptor = destination.descriptor();
   queue_nop(source, 102u);

   destination = cat::move(source);
   verify_empty(source);  // NOLINT(bugprone-use-after-move)
   verify_closed(previous_descriptor);
   source.close().verify();
   verify_open(source_descriptor);

   complete_nop(destination, 102u);
   destination.close().verify();
   verify_closed(source_descriptor);
   destination.close().verify();
}

$test(io_uring_self_move_assign) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::io_uring ring = cat::move(made).value();

   nix::file_descriptor const descriptor = ring.descriptor();
   cat::uint4 const entries = ring.params().sq_entries;
   queue_nop(ring, 103u);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
   ring = cat::move(ring);  // NOLINT
#pragma clang diagnostic pop

   cat::verify(ring.descriptor().value == descriptor.value);
   cat::verify(ring.params().sq_entries == entries);
   verify_open(descriptor);
   complete_nop(ring, 103u);

   ring.close().verify();
   verify_closed(descriptor);
   ring.close().verify();
}

$test(io_uring_swap) {
   auto left_made = make_ring();
   if (left_made.is_empty()) {
      return;
   }
   nix::io_uring left = cat::move(left_made).value();
   auto right_made = make_ring();
   if (right_made.is_empty()) {
      return;
   }
   nix::io_uring right = cat::move(right_made).value();

   nix::file_descriptor const left_descriptor = left.descriptor();
   nix::file_descriptor const right_descriptor = right.descriptor();
   queue_nop(left, 104u);
   queue_nop(right, 105u);

   left.swap(right);
   cat::verify(left.descriptor().value == right_descriptor.value);
   cat::verify(right.descriptor().value == left_descriptor.value);
   complete_nop(left, 105u);
   complete_nop(right, 104u);

   left.close().verify();
   verify_closed(right_descriptor);
   verify_open(left_descriptor);
   left.close().verify();

   right.close().verify();
   verify_closed(left_descriptor);
   right.close().verify();
}

// NOLINTEND(readability-magic-numbers)
