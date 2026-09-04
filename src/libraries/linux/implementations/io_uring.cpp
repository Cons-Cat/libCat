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

}  // namespace

auto
nix::make_io_uring(cat::uint4 entries, io_uring_params params)
   -> cat::scaredy<io_uring, linux_error> {
   io_uring ring;
   $prop(ring.setup(entries, params));  // NOLINT(misc-const-correctness)
   return cat::move(ring);
}

nix::io_uring::io_uring(io_uring&& other) {
   swap(other);
}

auto
nix::io_uring::operator=(io_uring&& other) -> io_uring& {
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
nix::io_uring::setup(cat::uint4 entries, io_uring_params params)
   -> scaredy_nix<void> {
   // TODO: Support caller-provided memory with `IORING_SETUP_NO_MMAP`.
   if (has_flag(params.flags, io_uring_setup_flags::no_mmap)) {
      return linux_error::inval;
   }

   // Keep partial setup local so each failure cleans it up.
   io_uring ring;
   ring.m_file_descriptor = $prop(sys_io_uring_setup(entries, params));
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
) -> scaredy_nix<cat::idx> {
   return sys_io_uring_enter(
      m_file_descriptor, to_submit, minimum_complete, flags
   );
}

auto
nix::io_uring::submit(io_uring_enter_flags flags) -> scaredy_nix<cat::idx> {
   return submit_and_wait(0u, flags);
}

auto
nix::io_uring::submit_and_wait(
   cat::uint4 minimum_complete, io_uring_enter_flags flags
) -> scaredy_nix<cat::idx> {
   cat::uint4 const submitted = m_submission_queue.flush();
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
nix::io_uring::close() -> scaredy_nix<void> {
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
   if (m_file_descriptor.value != invalid_file_descriptor.value) {
      file_descriptor const descriptor = m_file_descriptor;
      m_file_descriptor = invalid_file_descriptor;
      record_error(sys_close(descriptor));
   }

   m_params = {};
   m_submission_queue = {};
   m_completion_queue = {};
   return result;
}

void
nix::io_uring::swap(io_uring& other) {
   cat::swap(m_file_descriptor, other.m_file_descriptor);
   cat::swap(m_params, other.m_params);
   cat::swap(m_submission_queue, other.m_submission_queue);
   cat::swap(m_completion_queue, other.m_completion_queue);
   cat::swap(m_ring_mapping, other.m_ring_mapping);
   cat::swap(m_sqe_mapping, other.m_sqe_mapping);
}
