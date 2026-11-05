#include <cat/allocator_page>
#include <cat/raii_io_uring>

#include "../unit_tests.hpp"

// NOLINTBEGIN(readability-magic-numbers)

namespace {

cat::allocator_page ring_pager;

[[nodiscard]]
auto
make_ring(cat::uint4 entries = 8u, nix::io_uring_params params = {})
   -> cat::scaredy<nix::raii::io_uring<>, nix::linux_error> {
   auto made =
      nix::raii::make_io_uring(cat::allocator_dyn(ring_pager), entries, params);
   if (made.is_empty()) {
      cat::verify(
         made.error() == nix::linux_error::perm
         || made.error() == nix::linux_error::nosys
      );
   }
   return made;
}

[[nodiscard]]
auto
is_registered(nix::manual::io_uring const& ring) -> bool {
   return (ring.params().flags & nix::io_uring_setup_flags::registered_fd_only)
          != nix::io_uring_setup_flags::none;
}

void
verify_empty(nix::manual::io_uring const& ring) {
   cat::verify(ring.params().sq_entries == 0u);
   cat::verify(ring.params().cq_entries == 0u);
   cat::verify(ring.params().flags == nix::io_uring_setup_flags::none);
   cat::verify(ring.params().features == nix::io_uring_features::none);
}

void
verify_setup(
   nix::manual::io_uring const& ring, nix::io_uring_setup_flags flags
) {
   cat::verify(ring.params().sq_entries != 0u);
   cat::verify(ring.params().cq_entries != 0u);
   cat::verify(
      ring.params().flags
      == (flags | nix::io_uring_setup_flags::no_mmap | nix::io_uring_setup_flags::registered_fd_only)
   );
}

void
queue_nop(nix::manual::io_uring& ring, cat::uint8 user_data) {
   nix::io_uring_sqe* p_submission = ring.submission_queue().get_sqe();
   cat::verify(p_submission != nullptr);
   p_submission->opcode = nix::io_uring_opcode::nop;
   p_submission->user_data = user_data;
}

void
verify_nop_completion(nix::manual::io_uring& ring, cat::uint8 user_data) {
   nix::io_uring_cqe const* p_completion = ring.completion_queue().peek();
   cat::verify(p_completion != nullptr);
   cat::verify(p_completion->user_data == user_data);
   cat::verify(p_completion->result == 0);
   ring.completion_queue().advance();
}

void
complete_nop(nix::manual::io_uring& ring, cat::uint8 user_data) {
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

$test(io_uring_factory_and_queues) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> ring = cat::move(made).value();

   verify_setup(ring, nix::io_uring_setup_flags::none);
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

   ring.free();
   verify_empty(ring);
}

$test(io_uring_submit_paths) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> ring = cat::move(made).value();

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
   nix::raii::io_uring<> ring = cat::move(made).value();

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
   nix::raii::io_uring<> ring = cat::move(made).value();

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
   nix::raii::io_uring<> ring = cat::move(made).value();
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
   nix::raii::io_uring<> ring = cat::move(made).value();

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

$test(io_uring_multiple_pages) {
   // Queues spanning several pages need Linux 6.10.
   auto made = make_ring(4'096u);
   if (made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> ring = cat::move(made).value();
   cat::verify(ring.params().sq_entries == 4'096u);

   for (cat::idx index = 0u; index < 4'096u; ++index) {
      queue_nop(ring, 801u);
   }
   cat::verify(ring.submit_and_wait(4'096u).verify() == 4'096u);
   for (cat::idx index = 0u; index < 4'096u; ++index) {
      verify_nop_completion(ring, 801u);
   }
}

$test(io_uring_registered_slots_fallback) {
   // A thread has 16 registered ring slots. Later rings use a descriptor.
   cat::array<nix::manual::io_uring, 17u> rings;
   cat::idx made_count = 0u;
   $defer {
      for (cat::idx index = 0u; index < made_count; ++index) {
         rings[index].free(cat::allocator_dyn(ring_pager));
      }
   };
   for (nix::manual::io_uring& ring : rings) {
      auto made =
         nix::manual::make_io_uring(cat::allocator_dyn(ring_pager), 4u);
      if (made.is_empty()) {
         return;
      }
      ring = cat::move(made).value();
      ++made_count;
   }

   cat::verify(is_registered(rings[0]));
   cat::verify(!is_registered(rings[16]));
   for (nix::manual::io_uring& ring : rings) {
      queue_nop(ring, 901u);
      complete_nop(ring, 901u);
   }

   // Freeing a registered ring returns its slot.
   rings[0].free(cat::allocator_dyn(ring_pager));
   auto made = nix::manual::make_io_uring(cat::allocator_dyn(ring_pager), 4u);
   cat::verify(made.has_value());
   rings[0] = cat::move(made).value();
   cat::verify(is_registered(rings[0]));
}

$test(io_uring_free_and_destruct) {
   {
      auto made = make_ring();
      if (made.is_empty()) {
         return;
      }
      nix::raii::io_uring<> const ring = cat::move(made).value();
      verify_setup(ring, nix::io_uring_setup_flags::none);
   }

   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> ring = cat::move(made).value();
   queue_nop(ring, 151u);
   complete_nop(ring, 151u);
   ring.cfree();
   verify_empty(ring);
   ring.free();
   verify_empty(ring);
}

$test(io_uring_move_construct) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> source = cat::move(made).value();
   queue_nop(source, 101u);

   nix::raii::io_uring<> moved = cat::move(source);
   verify_empty(source);  // NOLINT(bugprone-use-after-move)
   source.free();

   complete_nop(moved, 101u);
   moved.free();
   verify_empty(moved);
   moved.free();
}

$test(io_uring_move_assign) {
   auto source_made = make_ring(4u);
   if (source_made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> source = cat::move(source_made).value();
   auto destination_made = make_ring(8u);
   if (destination_made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> destination = cat::move(destination_made).value();
   queue_nop(source, 102u);

   destination = cat::move(source);
   verify_empty(source);  // NOLINT(bugprone-use-after-move)
   cat::verify(destination.params().sq_entries == 4u);

   complete_nop(destination, 102u);
}

$test(io_uring_self_move_assign) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> ring = cat::move(made).value();
   cat::uint4 const entries = ring.params().sq_entries;
   queue_nop(ring, 103u);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
   ring = cat::move(ring);  // NOLINT
#pragma clang diagnostic pop

   cat::verify(ring.params().sq_entries == entries);
   complete_nop(ring, 103u);
}

$test(io_uring_swap) {
   auto left_made = make_ring(4u);
   if (left_made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> left = cat::move(left_made).value();
   auto right_made = make_ring(8u);
   if (right_made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> right = cat::move(right_made).value();
   queue_nop(left, 104u);
   queue_nop(right, 105u);

   left.swap(right);
   cat::verify(left.params().sq_entries == 8u);
   cat::verify(right.params().sq_entries == 4u);
   complete_nop(left, 105u);
   complete_nop(right, 104u);
}

namespace {

struct counted_operation : nix::io_uring_operation {
   cat::int4 result;
   cat::idx completions;
};

void
count_completion(
   nix::io_uring_operation& operation, nix::io_uring_cqe const& completion
) noexcept {
   auto& counted = static_cast<counted_operation&>(operation);
   counted.result = completion.result;
   ++counted.completions;
}

}  // namespace

$test(io_uring_dispatch_and_files) {
   auto made = make_ring();
   if (made.is_empty()) {
      return;
   }
   nix::raii::io_uring<> ring = cat::move(made).value();
   ring.register_file_table(4u).verify();

   // Fixed file slots are handed out and returned.
   nix::file_descriptor pipe[2];
   nix::sys_pipe(pipe).verify();
   cat::uint4 const read_slot = ring.register_file(pipe[0]).verify();
   cat::uint4 const write_slot = ring.register_file(pipe[1]).verify();
   cat::verify(read_slot != write_slot);

   // `dispatch()` hands each CQE to the operation in its `user_data`.
   counted_operation first{{count_completion}, -1, 0u};
   counted_operation second{{count_completion}, -1, 0u};
   for (counted_operation* p_operation : {&first, &second}) {
      nix::io_uring_sqe* p_submission = ring.submission_queue().get_sqe();
      p_submission->opcode = nix::io_uring_opcode::nop;
      p_submission->user_data = __builtin_bit_cast(cat::uword, p_operation);
   }
   ring.dispatch(2u).verify();
   cat::verify(first.completions == 1u && first.result == 0);
   cat::verify(second.completions == 1u && second.result == 0);

   ring.unregister_file(read_slot).verify();
   ring.unregister_file(write_slot).verify();
   nix::sys_close(pipe[0]).verify();
   nix::sys_close(pipe[1]).verify();
}

// NOLINTEND(readability-magic-numbers)
