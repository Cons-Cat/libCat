#include <cat/io_uring>

namespace {

template <typename T>
[[nodiscard]]
auto
ring_field(cat::span<cat::byte> mapping, cat::uint4 offset) -> T* {
   return __builtin_bit_cast(T*, mapping.data() + offset);
}

template <typename Flags>
[[nodiscard]]
constexpr auto
has_flag(Flags flags, Flags flag) -> bool {
   return (flags & flag) != Flags::none;
}

}  // namespace

nix::manual::io_uring::io_uring(io_uring&& other) {
   swap(other);
}

auto
nix::manual::io_uring::operator=(io_uring&& other) noexcept -> io_uring& {
   if (this == &other) {
      return *this;
   }

   cat::assert(
      m_ring.value == invalid_file_descriptor.value,
      "nix::manual::io_uring move-assigned over while still owning a ring. "
      "Call .free(allocator) first."
   );
   swap(other);
   return *this;
}

auto
nix::manual::io_uring::layout(
   cat::uint4 entries, io_uring_params const& params
) noexcept -> cat::scaredy<memory_layout, linux_error> {
   constexpr cat::uint4 max_entries = 32'768u;
   constexpr cat::uint4 max_completion_entries = max_entries * 2u;
   bool const is_clamped = has_flag(params.flags, io_uring_setup_flags::clamp);

   if (entries == 0u || (entries > max_entries && !is_clamped)) {
      return linux_error::inval;
   }
   cat::uint4 const sq_entries = cat::bit_ceil(cat::min(entries, max_entries));
   cat::uint4 cq_entries = sq_entries * 2u;
   if (has_flag(params.flags, io_uring_setup_flags::completion_size)) {
      if (
         params.cq_entries == 0u
         || (params.cq_entries > max_completion_entries && !is_clamped)
      ) {
         return linux_error::inval;
      }
      cq_entries =
         cat::bit_ceil(cat::min(params.cq_entries, max_completion_entries));
   }

   cat::idx const sqe_size =
      has_flag(params.flags, io_uring_setup_flags::sqe_128)
         ? sizeof(io_uring_sqe) * 2u
         : sizeof(io_uring_sqe);

   // Linux puts a cache line aligned header before the CQEs, and doubles the
   // whole size for 32 byte CQEs. Two cache lines bound that header.
   constexpr cat::idx header_size = 128u;
   cat::idx ring_size =
      header_size + cat::idx(cq_entries) * cat::idx(sizeof(io_uring_cqe));
   if (has_flag(params.flags, io_uring_setup_flags::cqe_32)) {
      ring_size *= 2u;
   }
   ring_size = cat::align_up(ring_size, header_size);
   if (!has_flag(params.flags, io_uring_setup_flags::no_sq_array)) {
      ring_size += sq_entries * cat::idx(sizeof(cat::uint4));
   }

   return memory_layout{
      .sqe_size = cat::align_up(sq_entries * sqe_size, cat::page_size),
      .ring_size = cat::align_up(ring_size, cat::page_size),
   };
}

auto
nix::manual::io_uring::setup(
   cat::span<cat::byte> memory, memory_layout layout, cat::uint4 entries,
   io_uring_params params
) noexcept(!__has_feature(cxx_exceptions)) -> scaredy_nix<void> {
   cat::byte* const p_sqes = memory.data();
   cat::byte* const p_rings = p_sqes + layout.sqe_size;
   params.flags |= io_uring_setup_flags::no_mmap;
   params.sq_off.user_address = __builtin_bit_cast(cat::uword, p_sqes);
   params.cq_off.user_address = __builtin_bit_cast(cat::uword, p_rings);

   // A thread only has 16 registered ring slots, so fall back to a descriptor
   // once those run out. This never exposes the descriptor, so the ring
   // behaves the same either way.
   io_uring_params attempt = params;
   attempt.flags |= io_uring_setup_flags::registered_fd_only;
   scaredy_nix<file_descriptor> made = sys_io_uring_setup(entries, attempt);
   if (made.is_empty() && made.error() == linux_error::busy) {
      attempt = params;
      made = sys_io_uring_setup(entries, attempt);
   }
   m_ring = $prop(made);
   m_enter_flags =
      has_flag(attempt.flags, io_uring_setup_flags::registered_fd_only)
         ? io_uring_enter_flags::registered_ring
         : io_uring_enter_flags::none;
   m_params = attempt;
   m_memory = memory;

   cat::idx const cqe_size =
      has_flag(attempt.flags, io_uring_setup_flags::cqe_32)
         ? sizeof(io_uring_cqe) * 2u
         : sizeof(io_uring_cqe);
   cat::idx ring_end = attempt.cq_off.cqes + attempt.cq_entries * cqe_size;
   if (!has_flag(attempt.flags, io_uring_setup_flags::no_sq_array)) {
      ring_end = cat::max(
         ring_end, attempt.sq_off.array
                      + attempt.sq_entries * cat::idx(sizeof(cat::uint4))
      );
   }

   // Linux would use memory past this allocation if `layout()` underestimated.
   if (ring_end > layout.ring_size) {
      auto const _ = destroy();
      return linux_error::overflow;
   }

   cat::span<cat::byte> const rings(p_rings, layout.ring_size);
   io_uring_submission_queue& submission_queue = m_submission_queue;
   submission_queue.m_p_head =
      ring_field<cat::uint4>(rings, attempt.sq_off.head);
   submission_queue.m_p_tail =
      ring_field<cat::uint4>(rings, attempt.sq_off.tail);
   submission_queue.m_p_flags =
      ring_field<cat::uint4>(rings, attempt.sq_off.flags);
   submission_queue.m_p_dropped =
      ring_field<cat::uint4>(rings, attempt.sq_off.dropped);

   if (!has_flag(attempt.flags, io_uring_setup_flags::no_sq_array)) {
      submission_queue.m_p_array =
         ring_field<cat::uint4>(rings, attempt.sq_off.array);
   }

   cat::idx const sqe_size =
      has_flag(attempt.flags, io_uring_setup_flags::sqe_128)
         ? sizeof(io_uring_sqe) * 2u
         : sizeof(io_uring_sqe);
   submission_queue.m_p_sqes = p_sqes;
   submission_queue.m_sqe_stride = sqe_size;
   submission_queue.m_ring_mask =
      *ring_field<cat::uint4>(rings, attempt.sq_off.ring_mask);
   submission_queue.m_ring_entries =
      *ring_field<cat::uint4>(rings, attempt.sq_off.ring_entries);
   submission_queue.m_sqe_head =
      cat::atomic<cat::uint4&>(*submission_queue.m_p_tail)
         .load(cat::memory_order::relaxed);
   submission_queue.m_sqe_tail = submission_queue.m_sqe_head;
   submission_queue.m_setup_flags = attempt.flags;

   io_uring_completion_queue& completion_queue = m_completion_queue;
   completion_queue.m_p_head =
      ring_field<cat::uint4>(rings, attempt.cq_off.head);
   completion_queue.m_p_tail =
      ring_field<cat::uint4>(rings, attempt.cq_off.tail);
   completion_queue.m_p_overflow =
      ring_field<cat::uint4>(rings, attempt.cq_off.overflow);
   completion_queue.m_p_cqes = p_rings + attempt.cq_off.cqes;
   completion_queue.m_cqe_stride = cqe_size;
   completion_queue.m_ring_mask =
      *ring_field<cat::uint4>(rings, attempt.cq_off.ring_mask);
   completion_queue.m_cqe_head =
      cat::atomic<cat::uint4&>(*completion_queue.m_p_head)
         .load(cat::memory_order::relaxed);
   completion_queue.m_setup_flags = attempt.flags;
   return cat::monostate;
}

auto
nix::manual::io_uring::enter(
   cat::uint4 to_submit, cat::uint4 minimum_complete, io_uring_enter_flags flags
) noexcept -> scaredy_nix<cat::idx> {
   return sys_io_uring_enter(
      m_ring, to_submit, minimum_complete, flags | m_enter_flags
   );
}

auto
nix::manual::io_uring::register_with_ring(
   io_uring_register_op operation, void* _Nullable p_argument, cat::uint4 count
) noexcept -> scaredy_nix<cat::idx> {
   if (m_enter_flags != io_uring_enter_flags::none) {
      operation = static_cast<io_uring_register_op>(
         cat::to_underlying(operation)
         | cat::to_underlying(io_uring_register_op::use_registered_ring)
      );
   }
   return sys_io_uring_register(m_ring, operation, p_argument, count);
}

auto
nix::manual::io_uring::submit(io_uring_enter_flags flags) noexcept
   -> scaredy_nix<cat::idx> {
   return submit_and_wait(0u, flags);
}

auto
nix::manual::io_uring::submit_and_wait(
   cat::uint4 minimum_complete, io_uring_enter_flags flags
) noexcept -> scaredy_nix<cat::idx> {
   cat::uint4 submitted = m_submission_queue.flush();

   // A failed `io_uring_enter` leaves published SQEs behind, which must be
   // submitted along with these. A polling thread consumes them on its own.
   if (
      !has_flag(m_params.flags, io_uring_setup_flags::sq_rewind)
      && !has_flag(m_params.flags, io_uring_setup_flags::sq_poll)
   ) {
      submitted = m_submission_queue.unconsumed_count();
   }

   if (minimum_complete != 0u) {
      flags |= io_uring_enter_flags::get_events;
   }

   if (has_flag(m_params.flags, io_uring_setup_flags::sq_poll)) {
      __atomic_thread_fence(cat::memory_order::seq_cst);

      if (m_submission_queue.needs_wakeup()) {
         flags |= io_uring_enter_flags::sq_wakeup;
      } else if (
         minimum_complete == 0u && flags == io_uring_enter_flags::none
      ) {
         return submitted;
      }
   }

   if (
      submitted == 0u && minimum_complete == 0u
      && flags == io_uring_enter_flags::none
   ) {
      return cat::idx{};
   }

   return enter(submitted, minimum_complete, flags);
}

auto
nix::manual::io_uring::destroy() noexcept(!__has_feature(cxx_exceptions))
   -> cat::span<cat::byte> {
   // These only fail for an invalid ring, which this owner never holds.
   if (m_enter_flags != io_uring_enter_flags::none) {
      // Registered ring slots belong to the thread, and outlive the ring.
      io_uring_resource_update update{
         .offset = m_ring.value,
         .data = 0u,
      };
      register_with_ring(io_uring_register_op::unregister_ring_fds, &update, 1u)
         .assert();
   } else if (m_ring.value != invalid_file_descriptor.value) {
      sys_close(m_ring).assert();
   }

   cat::span<cat::byte> memory = nullptr;
   memory.swap(m_memory);
   m_ring = invalid_file_descriptor;
   m_enter_flags = io_uring_enter_flags::none;
   m_params = {};
   m_submission_queue = {};
   m_completion_queue = {};
   return memory;
}

void
nix::manual::io_uring::swap(io_uring& other) {
   cat::swap(m_ring, other.m_ring);
   cat::swap(m_enter_flags, other.m_enter_flags);
   cat::swap(m_params, other.m_params);
   cat::swap(m_submission_queue, other.m_submission_queue);
   cat::swap(m_completion_queue, other.m_completion_queue);
   cat::swap(m_memory, other.m_memory);
}

auto
nix::manual::io_uring::dispatch(cat::uint4 minimum_complete) noexcept
   -> scaredy_nix<void> {
   auto const submitted = submit_and_wait(minimum_complete);
   if (
      submitted.is_empty() && submitted.error() != linux_error::intr
      && submitted.error() != linux_error::again
      && submitted.error() != linux_error::busy
   ) {
      return submitted.error();
   }
   while (io_uring_cqe const* p_completion = m_completion_queue.peek()) {
      // Copy the CQE, so that its slot is released before the handler might
      // submit and wait again.
      io_uring_cqe const completion = *p_completion;
      m_completion_queue.advance();
      auto* p_operation =
         __builtin_bit_cast(io_uring_operation*, completion.user_data);
      p_operation->p_complete(*p_operation, completion);
   }

   return cat::monostate;
}

auto
nix::manual::io_uring::register_file_table(cat::uint4 count) noexcept
   -> scaredy_nix<void> {
   io_uring_resource_register registration{
      .count = count,
      .flags = io_uring_resource_register_flags::sparse,
      .data = 0u,
      .tags = 0u,
   };
   $prop(register_with_ring(
      io_uring_register_op::register_files2, &registration, sizeof(registration)
   ));
   return cat::monostate;
}

auto
nix::manual::io_uring::register_file(file_descriptor descriptor) noexcept
   -> scaredy_nix<cat::uint4> {
   // Only the `files_update` operation can allocate a free slot. Linux writes
   // that slot's index over the descriptor.
   struct update_operation : io_uring_operation {
      cat::int4 result;
      bool is_complete;
   } update{
      {
       [](io_uring_operation& operation,
       io_uring_cqe const& completion) noexcept {
            auto& self = static_cast<update_operation&>(operation);
            self.result = completion.result;
            self.is_complete = true;
         }, },
      0,
      false,
   };

   cat::maybe<cat::int4> const descriptor_value =
      cat::narrow_cast<cat::int4>(descriptor.value);
   if (!descriptor_value.has_value()) {
      return linux_error::badf;
   }

   cat::int4 slot = descriptor_value.value();

   io_uring_sqe* p_submission = m_submission_queue.get_sqe();
   if (p_submission == nullptr) {
      $prop(submit());
      p_submission = m_submission_queue.get_sqe();
      if (p_submission == nullptr) {
         return linux_error::busy;
      }
   }

   p_submission->opcode = io_uring_opcode::files_update;
   p_submission->offset = io_uring_file_index_alloc;
   p_submission->address = __builtin_bit_cast(cat::uword, &slot);
   p_submission->length = 1u;
   p_submission->user_data = __builtin_bit_cast(cat::uword, &update);

   while (!update.is_complete) {
      $prop(dispatch(1u));
   }

   if (update.result < 0) {
      return static_cast<linux_error>(update.result);
   }

   // A successful update wrote a slot index, which is not negative.
   cat::maybe<cat::uint4> const index = cat::narrow_cast<cat::uint4>(slot);
   if (!index.has_value()) {
      return linux_error::proto;
   }

   return index.value();
}

auto
nix::manual::io_uring::unregister_file(cat::uint4 index) noexcept
   -> scaredy_nix<void> {
   cat::int4 empty = -1;
   io_uring_files_update update{
      .offset = index,
      .file_descriptors = __builtin_bit_cast(cat::uword, &empty),
   };
   $prop(register_with_ring(
      io_uring_register_op::register_files_update, &update, 1u
   ));
   return cat::monostate;
}
