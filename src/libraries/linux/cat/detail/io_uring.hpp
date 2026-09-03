// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

namespace nix {
// Linux 7.2 io_uring UAPI. Numeric domains remain open for later kernels.
// NOLINTBEGIN(performance-enum-size)

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_setup_flags : unsigned int {
   none = 0,
   io_poll = 1u << 0,
   sq_poll = 1u << 1,
   sq_affinity = 1u << 2,
   completion_size = 1u << 3,
   clamp = 1u << 4,
   attach_wq = 1u << 5,
   ring_disabled = 1u << 6,
   submit_all = 1u << 7,
   cooperative_taskrun = 1u << 8,
   taskrun_flag = 1u << 9,
   sqe_128 = 1u << 10,
   cqe_32 = 1u << 11,
   single_issuer = 1u << 12,
   defer_taskrun = 1u << 13,
   no_mmap = 1u << 14,
   registered_fd_only = 1u << 15,
   no_sq_array = 1u << 16,
   hybrid_io_poll = 1u << 17,
   cqe_mixed = 1u << 18,
   sqe_mixed = 1u << 19,
   sq_rewind = 1u << 20,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_features : unsigned int {
   none = 0,
   single_mmap = 1u << 0,
   no_drop = 1u << 1,
   submit_stable = 1u << 2,
   read_write_current_pos = 1u << 3,
   current_personality = 1u << 4,
   fast_poll = 1u << 5,
   poll_32_bits = 1u << 6,
   sq_poll_nonfixed = 1u << 7,
   ext_arg = 1u << 8,
   native_workers = 1u << 9,
   resource_tags = 1u << 10,
   cqe_skip = 1u << 11,
   linked_file = 1u << 12,
   registered_ring = 1u << 13,
   recvsend_bundle = 1u << 14,
   minimum_timeout = 1u << 15,
   read_write_attributes = 1u << 16,
   no_iowait = 1u << 17,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_enter_flags : unsigned int {
   none = 0,
   get_events = 1u << 0,
   sq_wakeup = 1u << 1,
   sq_wait = 1u << 2,
   ext_arg = 1u << 3,
   registered_ring = 1u << 4,
   absolute_timer = 1u << 5,
   ext_arg_registered = 1u << 6,
   no_iowait = 1u << 7,
};

enum class [[clang::enum_extensibility(
   open
)]] io_uring_register_op : unsigned int {
   register_buffers = 0,
   unregister_buffers = 1,
   register_files = 2,
   unregister_files = 3,
   register_eventfd = 4,
   unregister_eventfd = 5,
   register_files_update = 6,
   register_eventfd_async = 7,
   register_probe = 8,
   register_personality = 9,
   unregister_personality = 10,
   register_restrictions = 11,
   register_enable_rings = 12,
   register_files2 = 13,
   register_files_update2 = 14,
   register_buffers2 = 15,
   register_buffers_update = 16,
   register_iowq_aff = 17,
   unregister_iowq_aff = 18,
   register_iowq_max_workers = 19,
   register_ring_fds = 20,
   unregister_ring_fds = 21,
   register_pbuf_ring = 22,
   unregister_pbuf_ring = 23,
   register_sync_cancel = 24,
   register_file_alloc_range = 25,
   register_pbuf_status = 26,
   register_napi = 27,
   unregister_napi = 28,
   register_clock = 29,
   register_clone_buffers = 30,
   register_send_msg_ring = 31,
   register_zcrx_ifq = 32,
   register_resize_rings = 33,
   register_mem_region = 34,
   register_query = 35,
   register_zcrx_ctrl = 36,
   register_bpf_filter = 37,
   last = 38,
   use_registered_ring = 1u << 31,
};

struct io_sqring_offsets {
   cat::uint4 head;
   cat::uint4 tail;
   cat::uint4 ring_mask;
   cat::uint4 ring_entries;
   cat::uint4 flags;
   cat::uint4 dropped;
   cat::uint4 array;
   cat::uint4 _;
   cat::uint8 user_address;
};

static_assert(sizeof(io_sqring_offsets) == 40);

struct io_cqring_offsets {
   cat::uint4 head;
   cat::uint4 tail;
   cat::uint4 ring_mask;
   cat::uint4 ring_entries;
   cat::uint4 overflow;
   cat::uint4 cqes;
   cat::uint4 flags;
   cat::uint4 _;
   cat::uint8 user_address;
};

static_assert(sizeof(io_cqring_offsets) == 40);

struct io_uring_params {
   cat::uint4 sq_entries;
   cat::uint4 cq_entries;
   io_uring_setup_flags flags;
   cat::uint4 sq_thread_cpu;
   cat::uint4 sq_thread_idle;
   io_uring_features features;
   cat::uint4 wq_fd;
   cat::uint4 _[3];
   io_sqring_offsets sq_off;
   io_cqring_offsets cq_off;
};

static_assert(sizeof(io_uring_params) == 120);

inline constexpr cat::uint4 io_uring_file_index_alloc = 0xffffffffu;
inline constexpr cat::int4 io_uring_register_files_skip = -2;
inline constexpr cat::uint4 io_uring_notification_usage_zero_copy_copied =
   1u << 31;
inline constexpr cat::uint4 io_uring_cqe_buffer_shift = 16u;
inline constexpr cat::uint4 io_uring_pbuf_shift = 16u;

inline constexpr cat::uint8 io_uring_off_sq_ring = 0u;
inline constexpr cat::uint8 io_uring_off_cq_ring = 0x08000000u;
inline constexpr cat::uint8 io_uring_off_sqes = 0x10000000u;
inline constexpr cat::uint8 io_uring_off_pbuf_ring = 0x80000000u;
inline constexpr cat::uint8 io_uring_off_mmap_mask = 0xf8000000u;

enum class [[clang::enum_extensibility(open)]] io_uring_opcode : unsigned char {
   nop = 0,
   readv = 1,
   writev = 2,
   fsync = 3,
   read_fixed = 4,
   write_fixed = 5,
   poll_add = 6,
   poll_remove = 7,
   sync_file_range = 8,
   sendmsg = 9,
   recvmsg = 10,
   timeout = 11,
   timeout_remove = 12,
   accept = 13,
   async_cancel = 14,
   link_timeout = 15,
   connect = 16,
   fallocate = 17,
   openat = 18,
   close = 19,
   files_update = 20,
   statx = 21,
   read = 22,
   write = 23,
   fadvise = 24,
   madvise = 25,
   send = 26,
   recv = 27,
   openat2 = 28,
   epoll_ctl = 29,
   splice = 30,
   provide_buffers = 31,
   remove_buffers = 32,
   tee = 33,
   shutdown = 34,
   renameat = 35,
   unlinkat = 36,
   mkdirat = 37,
   symlinkat = 38,
   linkat = 39,
   msg_ring = 40,
   fsetxattr = 41,
   setxattr = 42,
   fgetxattr = 43,
   getxattr = 44,
   socket = 45,
   uring_command = 46,
   send_zero_copy = 47,
   sendmsg_zero_copy = 48,
   read_multishot = 49,
   waitid = 50,
   futex_wait = 51,
   futex_wake = 52,
   futex_waitv = 53,
   fixed_fd_install = 54,
   ftruncate = 55,
   bind = 56,
   listen = 57,
   recv_zero_copy = 58,
   epoll_wait = 59,
   readv_fixed = 60,
   writev_fixed = 61,
   pipe = 62,
   nop_128 = 63,
   uring_command_128 = 64,
   last = 65,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_sqe_flags : unsigned char {
   none = 0,
   fixed_file = 1u << 0,
   io_drain = 1u << 1,
   io_link = 1u << 2,
   io_hardlink = 1u << 3,
   async = 1u << 4,
   buffer_select = 1u << 5,
   cqe_skip_success = 1u << 6,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_command_flags : unsigned int {
   none = 0,
   fixed = 1u << 0,
   multishot = 1u << 1,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_fsync_flags : unsigned int {
   none = 0,
   data_sync = 1u << 0,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_timeout_flags : unsigned int {
   none = 0,
   absolute = 1u << 0,
   update = 1u << 1,
   boot_time = 1u << 2,
   realtime = 1u << 3,
   link_update = 1u << 4,
   etime_success = 1u << 5,
   multishot = 1u << 6,
   immediate_argument = 1u << 7,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_poll_flags : unsigned int {
   none = 0,
   multishot = 1u << 0,
   update_events = 1u << 1,
   update_user_data = 1u << 2,
   level = 1u << 3,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_splice_flags : unsigned int {
   none = 0,
   file_descriptor_in_fixed = 1u << 31,
};

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_async_cancel_flags : unsigned int {
   none = 0,
   all = 1u << 0,
   file_descriptor = 1u << 1,
   any = 1u << 2,
   fixed_file_descriptor = 1u << 3,
   user_data = 1u << 4,
   opcode = 1u << 5,
};

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_recvsend_flags : unsigned short {
   none = 0,
   poll_first = 1u << 0,
   recv_multishot = 1u << 1,
   fixed_buffer = 1u << 2,
   zero_copy_report_usage = 1u << 3,
   bundle = 1u << 4,
   vectorized = 1u << 5,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_accept_flags : unsigned short {
   none = 0,
   multishot = 1u << 0,
   dont_wait = 1u << 1,
   poll_first = 1u << 2,
};

enum class [[clang::enum_extensibility(
   open
)]] io_uring_msg_ring_command : unsigned int {
   data = 0,
   send_file_descriptor = 1,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_msg_ring_flags : unsigned int {
   none = 0,
   cqe_skip = 1u << 0,
   pass_flags = 1u << 1,
};

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_fixed_fd_install_flags : unsigned int {
   none = 0,
   no_close_on_exec = 1u << 0,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_nop_flags : unsigned int {
   none = 0,
   inject_result = 1u << 0,
   file = 1u << 1,
   fixed_file = 1u << 2,
   fixed_buffer = 1u << 3,
   task_work = 1u << 4,
   cqe_32 = 1u << 5,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_cqe_flags : unsigned int {
   none = 0,
   buffer = 1u << 0,
   more = 1u << 1,
   socket_nonempty = 1u << 2,
   notification = 1u << 3,
   buffer_more = 1u << 4,
   skip = 1u << 5,
   extended_32 = 1u << 15,
   timestamp_hardware = 1u << 16,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_sq_flags : unsigned int {
   none = 0,
   need_wakeup = 1u << 0,
   cq_overflow = 1u << 1,
   taskrun = 1u << 2,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_cq_flags : unsigned int {
   none = 0,
   eventfd_disabled = 1u << 0,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(
        open
     )]] io_uring_read_write_attribute_flags : unsigned long long {
   none = 0,
   protection_information = 1u << 0,
};

struct io_uring_sqe {
   io_uring_opcode opcode;
   io_uring_sqe_flags flags;
   cat::uint2 io_priority;
   cat::int4 file_descriptor;

   union {
      cat::uint8 offset;
      cat::uint8 address_2;

      struct {
         cat::uint4 command_opcode;
         cat::uint4 _;
      };
   };

   union {
      cat::uint8 address;
      cat::uint8 splice_offset_in;

      struct {
         cat::uint4 level;
         cat::uint4 option_name;
      };
   };

   cat::uint4 length;

   union {
      cat::uint4 read_write_flags;
      io_uring_fsync_flags fsync_flags;
      cat::uint2 poll_events;
      cat::uint4 poll_events_32;
      cat::uint4 sync_range_flags;
      cat::uint4 message_flags;
      io_uring_timeout_flags timeout_flags;
      cat::uint4 accept_flags;
      io_uring_async_cancel_flags cancel_flags;
      cat::uint4 open_flags;
      cat::uint4 statx_flags;
      cat::uint4 fadvise_advice;
      io_uring_splice_flags splice_flags;
      cat::uint4 rename_flags;
      cat::uint4 unlink_flags;
      cat::uint4 hardlink_flags;
      cat::uint4 xattr_flags;
      io_uring_msg_ring_flags msg_ring_flags;
      io_uring_command_flags uring_command_flags;
      cat::uint4 waitid_flags;
      cat::uint4 futex_flags;
      io_uring_fixed_fd_install_flags install_fd_flags;
      io_uring_nop_flags nop_flags;
      cat::uint4 pipe_flags;
   };

   cat::uint8 user_data;

   union {
      cat::uint2 buffer_index;
      cat::uint2 buffer_group;
   };

   cat::uint2 personality;

   union {
      cat::int4 splice_file_descriptor_in;
      cat::uint4 file_index;
      cat::uint4 zcrx_ifq_index;
      cat::uint4 option_length;

      struct {
         cat::uint2 address_length;
         cat::uint2 _[1];
      };

      struct {
         cat::uint1 write_stream;
         cat::uint1 _[3];
      };
   };

   union {
      struct {
         cat::uint8 address_3;
         cat::uint8 _[1];
      };

      struct {
         cat::uint8 attribute_pointer;
         io_uring_read_write_attribute_flags attribute_type_mask;
      };

      cat::uint8 option_value;
      cat::uint1 command[0];
   };
};

static_assert(sizeof(io_uring_sqe) == 64);
static_assert(alignof(io_uring_sqe) == 8);

struct io_uring_cqe {
   cat::uint8 user_data;
   cat::int4 result;
   io_uring_cqe_flags flags;
   cat::uint8 extended[];
};

static_assert(sizeof(io_uring_cqe) == 16);
static_assert(alignof(io_uring_cqe) == 8);

struct io_uring_attr_pi {
   cat::uint2 flags;
   cat::uint2 application_tag;
   cat::uint4 length;
   cat::uint8 address;
   cat::uint8 seed;
   cat::uint8 _;
};

static_assert(sizeof(io_uring_attr_pi) == 32);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_worker_type : unsigned int {
   bound = 0,
   unbound = 1,
};

struct io_uring_files_update {
   cat::uint4 offset;
   cat::uint4 _;
   cat::uint8 file_descriptors;
};

static_assert(sizeof(io_uring_files_update) == 16);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_memory_region_type : unsigned int {
   user = 1,
};

struct io_uring_region_descriptor {
   cat::uint8 user_address;
   cat::uint8 size;
   cat::uint4 flags;
   cat::uint4 id;
   cat::uint8 mmap_offset;
   cat::uint8 _[4];
};

static_assert(sizeof(io_uring_region_descriptor) == 64);

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_memory_region_flags : unsigned long long {
   none = 0,
   registered_wait_argument = 1u << 0,
};

struct io_uring_memory_region_registration {
   cat::uint8 region_pointer;
   io_uring_memory_region_flags flags;
   cat::uint8 _[2];
};

static_assert(sizeof(io_uring_memory_region_registration) == 32);

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_resource_register_flags : unsigned int {
   none = 0,
   sparse = 1u << 0,
};

struct io_uring_resource_register {
   cat::uint4 count;
   io_uring_resource_register_flags flags;
   cat::uint8 _;
   cat::uint8 data;
   cat::uint8 tags;
};

static_assert(sizeof(io_uring_resource_register) == 32);

struct io_uring_resource_update {
   cat::uint4 offset;
   cat::uint4 _;
   cat::uint8 data;
};

static_assert(sizeof(io_uring_resource_update) == 16);

struct io_uring_resource_update_2 {
   cat::uint4 offset;
   cat::uint4 _;
   cat::uint8 data;
   cat::uint8 tags;
   cat::uint4 count;
   cat::uint4 _;
};

static_assert(sizeof(io_uring_resource_update_2) == 32);

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_probe_flags : unsigned short {
   none = 0,
   supported = 1u << 0,
};

struct io_uring_probe_opcode {
   io_uring_opcode opcode;
   cat::uint1 _;
   io_uring_probe_flags flags;
   cat::uint4 _;
};

static_assert(sizeof(io_uring_probe_opcode) == 8);

struct io_uring_probe {
   io_uring_opcode last_opcode;
   cat::uint1 operations_length;
   cat::uint2 _;
   cat::uint4 _[3];
   io_uring_probe_opcode operations[];
};

static_assert(sizeof(io_uring_probe) == 16);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_restriction_opcode : unsigned short {
   register_opcode = 0,
   sqe_opcode = 1,
   sqe_flags_allowed = 2,
   sqe_flags_required = 3,
   last = 4,
};

struct io_uring_restriction {
   io_uring_restriction_opcode opcode;

   union {
      cat::uint1 register_opcode;
      io_uring_opcode sqe_opcode;
      io_uring_sqe_flags sqe_flags;
   };

   cat::uint1 _;
   cat::uint4 _[3];
};

static_assert(sizeof(io_uring_restriction) == 16);

struct io_uring_task_restriction {
   cat::uint2 flags;
   cat::uint2 restriction_count;
   cat::uint4 _[3];
   io_uring_restriction restrictions[];
};

static_assert(sizeof(io_uring_task_restriction) == 16);

struct io_uring_clock_register {
   cat::uint4 clock_id;
   cat::uint4 _[3];
};

static_assert(sizeof(io_uring_clock_register) == 16);

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_clone_buffer_flags : unsigned int {
   none = 0,
   source_registered = 1u << 0,
   destination_replace = 1u << 1,
};

struct io_uring_clone_buffers {
   cat::uint4 source_file_descriptor;
   io_uring_clone_buffer_flags flags;
   cat::uint4 source_offset;
   cat::uint4 destination_offset;
   cat::uint4 count;
   cat::uint4 _[3];
};

static_assert(sizeof(io_uring_clone_buffers) == 32);

struct io_uring_buffer {
   cat::uint8 address;
   cat::uint4 length;
   cat::uint2 id;
   cat::uint2 _;
};

static_assert(sizeof(io_uring_buffer) == 16);

struct io_uring_buffer_ring {
   union {
      struct {
         cat::uint8 _;
         cat::uint4 _;
         cat::uint2 _;
         cat::uint2 tail;
      };

      io_uring_buffer buffers[0];
   };
};

static_assert(sizeof(io_uring_buffer_ring) == 16);

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_buffer_ring_flags : unsigned short {
   none = 0,
   mmap = 1u << 0,
   incremental = 1u << 1,
};

struct io_uring_buffer_register {
   cat::uint8 ring_address;
   cat::uint4 ring_entries;
   cat::uint2 buffer_group_id;
   io_uring_buffer_ring_flags flags;
   cat::uint4 minimum_left;
   cat::uint4 _[5];
};

static_assert(sizeof(io_uring_buffer_register) == 40);

struct io_uring_buffer_status {
   cat::uint4 buffer_group;
   cat::uint4 head;
   cat::uint4 _[8];
};

static_assert(sizeof(io_uring_buffer_status) == 40);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_napi_opcode : unsigned char {
   register_settings = 0,
   static_add_id = 1,
   static_delete_id = 2,
};

enum class [[clang::enum_extensibility(
   open
)]] io_uring_napi_tracking : unsigned int {
   dynamic = 0,
   static_set = 1,
   inactive = 255,
};

struct io_uring_napi {
   cat::uint4 busy_poll_timeout;
   cat::uint1 prefer_busy_poll;
   io_uring_napi_opcode opcode;
   cat::uint1 _[2];
   cat::uint4 opcode_parameter;
   cat::uint4 _;
};

static_assert(sizeof(io_uring_napi) == 16);

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_wait_flags : unsigned int {
   none = 0,
   timespec = 1u << 0,
};

struct io_uring_timespec {
   cat::int8 seconds;
   cat::int8 nanoseconds;
};

static_assert(sizeof(io_uring_timespec) == 16);

struct io_uring_registered_wait {
   io_uring_timespec timeout;
   cat::uint4 minimum_wait_microseconds;
   io_uring_wait_flags flags;
   cat::uint8 signal_mask;
   cat::uint4 signal_mask_size;
   cat::uint4 _[3];
   cat::uint8 _[2];
};

static_assert(sizeof(io_uring_registered_wait) == 64);

struct io_uring_getevents_argument {
   cat::uint8 signal_mask;
   cat::uint4 signal_mask_size;
   cat::uint4 minimum_wait_microseconds;
   cat::uint8 timeout;
};

static_assert(sizeof(io_uring_getevents_argument) == 24);

struct io_uring_sync_cancel_register {
   cat::uint8 address;
   cat::int4 file_descriptor;
   io_uring_async_cancel_flags flags;
   io_uring_timespec timeout;
   io_uring_opcode opcode;
   cat::uint1 _[7];
   cat::uint8 _[3];
};

static_assert(sizeof(io_uring_sync_cancel_register) == 64);

struct io_uring_file_index_range {
   cat::uint4 offset;
   cat::uint4 length;
   cat::uint8 _;
};

static_assert(sizeof(io_uring_file_index_range) == 16);

struct io_uring_recvmsg_output {
   cat::uint4 name_length;
   cat::uint4 control_length;
   cat::uint4 payload_length;
   cat::uint4 flags;
};

static_assert(sizeof(io_uring_recvmsg_output) == 16);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_socket_command : unsigned int {
   input_queue_size = 0,
   output_queue_size = 1,
   get_option = 2,
   set_option = 3,
   transmit_timestamp = 4,
   get_name = 5,
};

inline constexpr cat::uint4 io_uring_timestamp_hardware_shift = 16u;
inline constexpr cat::uint4 io_uring_timestamp_type_shift = 17u;

struct io_uring_io_timespec {
   cat::uint8 seconds;
   cat::uint8 nanoseconds;
};

static_assert(sizeof(io_uring_io_timespec) == 16);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_query_opcode : unsigned int {
   opcodes = 0,
   zcrx = 1,
   shared_completion_queue = 2,
   zcrx_event = 3,
   maximum = 4,
};

struct io_uring_query_header {
   cat::uint8 next_entry;
   cat::uint8 query_data;
   io_uring_query_opcode query_opcode;
   cat::uint4 size;
   cat::int4 result;
   cat::uint4 _[3];
};

static_assert(sizeof(io_uring_query_header) == 40);

struct io_uring_query_opcodes {
   cat::uint4 request_opcode_count;
   cat::uint4 register_opcode_count;
   cat::uint8 feature_flags;
   cat::uint8 ring_setup_flags;
   cat::uint8 enter_flags;
   cat::uint8 sqe_flags;
   cat::uint4 query_opcode_count;
   cat::uint4 _;
};

static_assert(sizeof(io_uring_query_opcodes) == 48);

struct io_uring_query_zcrx {
   cat::uint8 register_flags;
   cat::uint8 area_flags;
   cat::uint4 control_opcode_count;
   cat::uint4 features;
   cat::uint4 refill_header_size;
   cat::uint4 refill_header_alignment;
   cat::uint8 _;
};

static_assert(sizeof(io_uring_query_zcrx) == 40);

struct io_uring_query_zcrx_event {
   cat::uint4 event_flags;
   cat::uint4 statistics_size;
   cat::uint4 statistics_offset_alignment;
   cat::uint4 _;
   cat::uint8 _[4];
};

static_assert(sizeof(io_uring_query_zcrx_event) == 48);

struct io_uring_query_shared_completion_queue {
   cat::uint8 header_size;
   cat::uint8 header_alignment;
};

static_assert(sizeof(io_uring_query_shared_completion_queue) == 16);

struct io_uring_zcrx_refill_entry {
   cat::uint8 offset;
   cat::uint4 length;
   cat::uint4 _;
};

static_assert(sizeof(io_uring_zcrx_refill_entry) == 16);

struct io_uring_zcrx_completion {
   cat::uint8 offset;
   cat::uint8 _;
};

static_assert(sizeof(io_uring_zcrx_completion) == 16);

inline constexpr cat::uint4 io_uring_zcrx_area_shift = 48u;
inline constexpr cat::uint8 io_uring_zcrx_area_mask = 0xffff0000'00000000u;

struct io_uring_zcrx_offsets {
   cat::uint4 head;
   cat::uint4 tail;
   cat::uint4 refill_entries;
   cat::uint4 _;
   cat::uint8 _[2];
};

static_assert(sizeof(io_uring_zcrx_offsets) == 32);

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_zcrx_area_flags : unsigned int {
   none = 0,
   dma_buffer = 1u << 0,
};

struct io_uring_zcrx_area_register {
   cat::uint8 address;
   cat::uint8 length;
   cat::uint8 refill_area_token;
   io_uring_zcrx_area_flags flags;
   cat::uint4 dma_buffer_file_descriptor;
   cat::uint8 _[2];
};

static_assert(sizeof(io_uring_zcrx_area_register) == 48);

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_zcrx_register_flags : unsigned int {
   none = 0,
   import = 1u << 0,
   no_device = 1u << 1,
};

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_zcrx_features : unsigned int {
   none = 0,
   receive_page_size = 1u << 0,
   event = 1u << 1,
};

enum class [[clang::enum_extensibility(
   open
)]] io_uring_zcrx_event_type : unsigned int {
   allocation_failure = 0,
   copy = 1,
   last = 2,
};

enum class
   [[clang::flag_enum, clang::enum_extensibility(
                          open
                       )]] io_uring_zcrx_event_descriptor_flags : unsigned int {
   none = 0,
   statistics = 1u << 0,
};

struct io_uring_zcrx_statistics {
   cat::uint8 copy_count;
   cat::uint8 copy_bytes;
};

static_assert(sizeof(io_uring_zcrx_statistics) == 16);

struct io_uring_zcrx_event_descriptor {
   cat::uint8 user_data;
   cat::uint4 type_mask;
   io_uring_zcrx_event_descriptor_flags flags;
   cat::uint8 statistics_offset;
   cat::uint8 _[9];
};

static_assert(sizeof(io_uring_zcrx_event_descriptor) == 96);

struct io_uring_zcrx_ifq_register {
   cat::uint4 interface_index;
   cat::uint4 interface_receive_queue;
   cat::uint4 refill_queue_entries;
   io_uring_zcrx_register_flags flags;
   cat::uint8 area_pointer;
   cat::uint8 region_pointer;
   io_uring_zcrx_offsets offsets;
   cat::uint4 zcrx_id;
   cat::uint4 receive_buffer_length;
   cat::uint8 event_descriptor;
   cat::uint8 _[2];
};

static_assert(sizeof(io_uring_zcrx_ifq_register) == 96);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_zcrx_control_opcode : unsigned int {
   flush_refill_queue = 0,
   export_queue = 1,
   arm_event = 2,
   last = 3,
};

struct io_uring_zcrx_control_flush {
   cat::uint8 _[6];
};

struct io_uring_zcrx_control_export {
   cat::uint4 zcrx_file_descriptor;
   cat::uint4 _[11];
};

struct io_uring_zcrx_control_arm_event {
   io_uring_zcrx_event_type event_type;
   cat::uint4 _[11];
};

static_assert(sizeof(io_uring_zcrx_control_flush) == 48);
static_assert(sizeof(io_uring_zcrx_control_export) == 48);
static_assert(sizeof(io_uring_zcrx_control_arm_event) == 48);

struct io_uring_zcrx_control {
   cat::uint4 zcrx_id;
   io_uring_zcrx_control_opcode opcode;
   cat::uint8 _[2];

   union {
      io_uring_zcrx_control_export export_queue;
      io_uring_zcrx_control_flush flush;
      io_uring_zcrx_control_arm_event arm_event;
   };
};

static_assert(sizeof(io_uring_zcrx_control) == 72);

struct io_uring_bpf_context {
   cat::uint8 user_data;
   io_uring_opcode opcode;
   io_uring_sqe_flags sqe_flags;
   cat::uint1 payload_size;
   cat::uint1 _[5];

   union {
      struct {
         cat::uint4 family;
         cat::uint4 type;
         cat::uint4 protocol;
      } socket;

      struct {
         cat::uint8 flags;
         cat::uint8 mode;
         cat::uint8 resolve;
      } open;

      struct {
         cat::uint4 family;
         cat::uint2 port;
         cat::uint1 _[2];

         union {
            cat::uint4 ipv4_address;
            cat::uint1 ipv6_address[16];
         };
      } connect;
   };
};

static_assert(sizeof(io_uring_bpf_context) == 40);

enum class
   [[clang::flag_enum,
     clang::enum_extensibility(open)]] io_uring_bpf_flags : unsigned int {
   none = 0,
   deny_rest = 1u << 0,
   strict_size = 1u << 1,
};

struct io_uring_bpf_filter {
   cat::uint4 opcode;
   io_uring_bpf_flags flags;
   cat::uint4 filter_length;
   cat::uint1 payload_size;
   cat::uint1 _[3];
   cat::uint8 filter_pointer;
   cat::uint8 _[5];
};

static_assert(sizeof(io_uring_bpf_filter) == 64);

enum class [[clang::enum_extensibility(
   open
)]] io_uring_bpf_command : unsigned short {
   filter = 1,
};

struct io_uring_bpf {
   io_uring_bpf_command command;
   cat::uint2 command_flags;
   cat::uint4 _;

   union {
      io_uring_bpf_filter filter;
   };
};

static_assert(sizeof(io_uring_bpf) == 72);
// NOLINTEND(performance-enum-size)
}  // namespace nix

template <>
struct cat::enum_flag_trait<nix::io_uring_sqe_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_command_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_fsync_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_timeout_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_poll_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_splice_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_async_cancel_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_recvsend_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_accept_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_msg_ring_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_fixed_fd_install_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_nop_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_cqe_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_sq_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_cq_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_read_write_attribute_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_memory_region_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_resource_register_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_probe_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_clone_buffer_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_buffer_ring_flags> : cat::true_trait {
};

template <>
struct cat::enum_flag_trait<nix::io_uring_wait_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_zcrx_area_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_zcrx_register_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_zcrx_features> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_zcrx_event_descriptor_flags>
    : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_bpf_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_setup_flags> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_features> : cat::true_trait {};

template <>
struct cat::enum_flag_trait<nix::io_uring_enter_flags> : cat::true_trait {};
