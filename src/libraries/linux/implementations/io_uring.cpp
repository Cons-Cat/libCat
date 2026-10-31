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

[[nodiscard]]
auto
map_ring(nix::file_descriptor file_descriptor, cat::idx size, cat::uint8 offset)
   -> cat::scaredy<cat::span<cat::byte>, nix::linux_error> {
   cat::byte* p_mapping = $prop(
      nix::sys_mmap(
         nullptr, size, nix::memory_protection_flags::read_write,
         nix::memory_flags::shared | nix::memory_flags::populate,
         file_descriptor, offset / cat::page_size
      )
   );
   return cat::span<cat::byte>(p_mapping, size);
}

// The calling thread's shared ring lives here once allocated.
struct alignas(nix::io_uring) io_uring_runtime_storage {
   cat::array<cat::byte, sizeof(nix::io_uring)> bytes;
};

constinit thread_local io_uring_runtime_storage io_uring_runtime_slot{};

}  // namespace

auto
nix::make_io_uring(cat::uint4 entries, io_uring_params params) noexcept(
   !__has_feature(cxx_exceptions)
) -> cat::scaredy<io_uring, linux_error> {
   io_uring ring;
   $prop(ring.setup(entries, params));  // NOLINT(misc-const-correctness)
   return cat::move(ring);
}

nix::io_uring::io_uring(io_uring&& other) {
   swap(other);
}

auto
nix::io_uring::operator=(io_uring&& other) noexcept -> io_uring& {
   if (this == &other) {
      return *this;
   }

   io_uring previous;
   previous.swap(*this);
   swap(other);
   return *this;
}

nix::io_uring::~io_uring() {
   auto const _ = close();
}

auto
nix::io_uring::setup(cat::uint4 entries, io_uring_params params) noexcept(
   !__has_feature(cxx_exceptions)
) -> scaredy_nix<void> {
   // TODO: Support caller-provided memory with `IORING_SETUP_NO_MMAP`.
   if (has_flag(params.flags, io_uring_setup_flags::no_mmap)) {
      return linux_error::inval;
   }

   // Keep partial setup local so each failure cleans it up.
   io_uring ring;
   ring.m_file_descriptor = $prop(sys_io_uring_setup(entries, params));
   ring.m_enter_descriptor = ring.m_file_descriptor;
   ring.m_params = params;
   if (!has_flag(params.features, io_uring_features::single_mmap)) {
      return linux_error::opnotsupp;
   }

   cat::idx const cqe_size =
      has_flag(params.flags, io_uring_setup_flags::cqe_32)
         ? sizeof(io_uring_cqe) * 2u
         : sizeof(io_uring_cqe);
   cat::idx const completion_mapping_size =
      params.cq_off.cqes + params.cq_entries * cqe_size;
   cat::idx submission_mapping_size =
      params.sq_off.array + params.sq_entries * cat::idx(sizeof(cat::uint4));
   if (has_flag(params.flags, io_uring_setup_flags::no_sq_array)) {
      submission_mapping_size = completion_mapping_size;
   }

   cat::idx const ring_mapping_size =
      submission_mapping_size < completion_mapping_size
         ? completion_mapping_size
         : submission_mapping_size;
   ring.m_ring_mapping = $prop(
      map_ring(ring.m_file_descriptor, ring_mapping_size, io_uring_off_sq_ring)
   );

   cat::idx const sqe_size =
      has_flag(params.flags, io_uring_setup_flags::sqe_128)
         ? sizeof(io_uring_sqe) * 2u
         : sizeof(io_uring_sqe);
   cat::idx const sqe_mapping_size = params.sq_entries * sqe_size;
   ring.m_sqe_mapping = $prop(
      map_ring(ring.m_file_descriptor, sqe_mapping_size, io_uring_off_sqes)
   );

   io_uring_submission_queue& submission_queue = ring.m_submission_queue;
   submission_queue.m_p_head =
      ring_field<cat::uint4>(ring.m_ring_mapping, params.sq_off.head);
   submission_queue.m_p_tail =
      ring_field<cat::uint4>(ring.m_ring_mapping, params.sq_off.tail);
   submission_queue.m_p_flags =
      ring_field<cat::uint4>(ring.m_ring_mapping, params.sq_off.flags);
   submission_queue.m_p_dropped =
      ring_field<cat::uint4>(ring.m_ring_mapping, params.sq_off.dropped);
   if (!has_flag(params.flags, io_uring_setup_flags::no_sq_array)) {
      submission_queue.m_p_array =
         ring_field<cat::uint4>(ring.m_ring_mapping, params.sq_off.array);
   }
   submission_queue.m_p_sqes = ring.m_sqe_mapping.data();
   submission_queue.m_sqe_stride = sqe_size;
   submission_queue.m_ring_mask =
      *ring_field<cat::uint4>(ring.m_ring_mapping, params.sq_off.ring_mask);
   submission_queue.m_ring_entries =
      *ring_field<cat::uint4>(ring.m_ring_mapping, params.sq_off.ring_entries);
   submission_queue.m_sqe_head =
      cat::atomic<cat::uint4&>(*submission_queue.m_p_tail)
         .load(cat::memory_order::relaxed);
   submission_queue.m_sqe_tail = submission_queue.m_sqe_head;
   submission_queue.m_setup_flags = params.flags;

   io_uring_completion_queue& completion_queue = ring.m_completion_queue;
   completion_queue.m_p_head =
      ring_field<cat::uint4>(ring.m_ring_mapping, params.cq_off.head);
   completion_queue.m_p_tail =
      ring_field<cat::uint4>(ring.m_ring_mapping, params.cq_off.tail);
   completion_queue.m_p_overflow =
      ring_field<cat::uint4>(ring.m_ring_mapping, params.cq_off.overflow);
   completion_queue.m_p_cqes = ring.m_ring_mapping.data() + params.cq_off.cqes;
   completion_queue.m_cqe_stride = cqe_size;
   completion_queue.m_ring_mask =
      *ring_field<cat::uint4>(ring.m_ring_mapping, params.cq_off.ring_mask);
   completion_queue.m_cqe_head =
      cat::atomic<cat::uint4&>(*completion_queue.m_p_head)
         .load(cat::memory_order::relaxed);
   completion_queue.m_setup_flags = params.flags;

   *this = cat::move(ring);
   return cat::monostate;
}

auto
nix::io_uring::enter(
   cat::uint4 to_submit, cat::uint4 minimum_complete, io_uring_enter_flags flags
) noexcept -> scaredy_nix<cat::idx> {
   return sys_io_uring_enter(
      m_enter_descriptor, to_submit, minimum_complete, flags | m_enter_flags
   );
}

auto
nix::io_uring::register_ring_descriptor() noexcept -> scaredy_nix<void> {
   io_uring_resource_update update{
      .offset = io_uring_file_index_alloc,
      .data = m_file_descriptor.value,
   };
   $prop(sys_io_uring_register(
      m_file_descriptor, io_uring_register_op::register_ring_fds, &update, 1u
   ));
   m_enter_descriptor = update.offset;
   m_enter_flags = io_uring_enter_flags::registered_ring;
   return cat::monostate;
}

auto
nix::io_uring::submit(io_uring_enter_flags flags) noexcept
   -> scaredy_nix<cat::idx> {
   return submit_and_wait(0u, flags);
}

auto
nix::io_uring::submit_and_wait(
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
nix::io_uring::close() noexcept(!__has_feature(cxx_exceptions))
   -> scaredy_nix<void> {
   scaredy_nix<void> result = cat::monostate;
   auto const record_error = [&result](scaredy_nix<void> current) {
      if (result.has_value() && current.is_empty()) {
         result = current.error();
      }
   };

   if (m_sqe_mapping.data() != nullptr) {
      cat::span<cat::byte> mapping(nullptr);
      mapping.swap(m_sqe_mapping);
      record_error(sys_munmap(mapping.data(), mapping.size()));
   }
   if (m_ring_mapping.data() != nullptr) {
      cat::span<cat::byte> mapping(nullptr);
      mapping.swap(m_ring_mapping);
      record_error(sys_munmap(mapping.data(), mapping.size()));
   }
   if (m_enter_flags != io_uring_enter_flags::none) {
      // Registered ring slots belong to the thread, and outlive the ring.
      io_uring_resource_update update{.offset = m_enter_descriptor.value};
      auto const unregistered = sys_io_uring_register(
         m_file_descriptor, io_uring_register_op::unregister_ring_fds, &update,
         1u
      );
      if (result.has_value() && unregistered.is_empty()) {
         result = unregistered.error();
      }
   }
   if (m_file_descriptor.value != invalid_file_descriptor.value) {
      file_descriptor const descriptor = m_file_descriptor;
      m_file_descriptor = invalid_file_descriptor;
      record_error(sys_close(descriptor));
   }
   m_enter_descriptor = invalid_file_descriptor;
   m_enter_flags = io_uring_enter_flags::none;

   m_params = {};
   m_submission_queue = {};
   m_completion_queue = {};
   return result;
}

void
nix::io_uring::swap(io_uring& other) {
   cat::swap(m_file_descriptor, other.m_file_descriptor);
   cat::swap(m_enter_descriptor, other.m_enter_descriptor);
   cat::swap(m_enter_flags, other.m_enter_flags);
   cat::swap(m_params, other.m_params);
   cat::swap(m_submission_queue, other.m_submission_queue);
   cat::swap(m_completion_queue, other.m_completion_queue);
   cat::swap(m_ring_mapping, other.m_ring_mapping);
   cat::swap(m_sqe_mapping, other.m_sqe_mapping);
}

auto
nix::io_uring::dispatch(cat::uint4 minimum_complete) noexcept
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
nix::io_uring::register_file_table(cat::uint4 count) noexcept
   -> scaredy_nix<void> {
   io_uring_resource_register registration{
      .count = count,
      .flags = io_uring_resource_register_flags::sparse,
   };
   $prop(sys_io_uring_register(
      m_file_descriptor, io_uring_register_op::register_files2, &registration,
      sizeof(registration)
   ));
   return cat::monostate;
}

auto
nix::io_uring::register_file(file_descriptor descriptor) noexcept
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
nix::io_uring::unregister_file(cat::uint4 index) noexcept -> scaredy_nix<void> {
   cat::int4 empty = -1;
   io_uring_files_update update{
      .offset = index,
      .file_descriptors = __builtin_bit_cast(cat::uword, &empty),
   };
   $prop(sys_io_uring_register(
      m_file_descriptor, io_uring_register_op::register_files_update, &update,
      1u
   ));
   return cat::monostate;
}

auto
nix::alloc_io_uring_runtime(cat::uint4 entries, cat::uint4 files) noexcept
   -> scaredy_nix<void> {
   if (detail::p_io_uring_runtime != nullptr) {
      return cat::monostate;
   }
   // `SINGLE_ISSUER` and `DEFER_TASKRUN` run completions only inside this
   // thread's `io_uring_enter`, without internal locking. `TASKRUN_FLAG`
   // reports when there are any to run, so callers can skip entering.
   io_uring_params params{};
   params.flags = io_uring_setup_flags::single_issuer
                  | io_uring_setup_flags::defer_taskrun
                  | io_uring_setup_flags::taskrun_flag
                  | io_uring_setup_flags::submit_all
                  | io_uring_setup_flags::no_sq_array;
   io_uring ring = $prop(make_io_uring(entries, params));
   $prop(ring.register_file_table(files));
   // Skip the descriptor table lookup on every `io_uring_enter`.
   $prop(ring.register_ring_descriptor());
   detail::p_io_uring_runtime =
      new (io_uring_runtime_slot.bytes.data()) io_uring(cat::move(ring));
   return cat::monostate;
}

void
nix::free_io_uring_runtime() noexcept {
   if (detail::p_io_uring_runtime != nullptr) {
      detail::p_io_uring_runtime->~io_uring();
      detail::p_io_uring_runtime = nullptr;
   }
}
