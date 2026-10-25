#include <cat/detail/machine_pause.hpp>

#include <cat/atomic>
#include <cat/exceptions>
#include <cat/new>

#include "implementations/narrow_cast.tpp"

namespace cat {
namespace {

inline constexpr uint1 allocator_state_empty = 0u;
inline constexpr uint1 allocator_state_initializing = 1u;
inline constexpr uint1 allocator_state_ready = 2u;

struct alignas(allocator_dyn) exception_allocator_storage {
   byte bytes[sizeof(allocator_dyn)]{};
};

inline constinit atomic<uint1> exception_allocator_state{};
inline constinit exception_allocator_storage exception_allocator_slot{};

auto
exception_allocator() -> allocator_dyn* _Nullable {
   if (exception_allocator_state.acquire() != allocator_state_ready) {
      return nullptr;
   }

   return __builtin_launder(
      reinterpret_cast<allocator_dyn*>(exception_allocator_slot.bytes)
   );
}

}  // namespace

namespace detail {

void
register_exception_allocator(allocator_dyn allocator) {
   if (exception_allocator_state.acquire() == allocator_state_ready) {
      return;
   }

   uint1 expected = allocator_state_empty;
   if (!exception_allocator_state.acq_rel().compare_exchange_strong(
          expected, allocator_state_initializing
       )) {
      while (expected != allocator_state_ready) {
         machine_pause();
         expected = exception_allocator_state.acquire();
      }
      return;
   }

   new (exception_allocator_slot.bytes) allocator_dyn(allocator);
   exception_allocator_state.release() = allocator_state_ready;
}

}  // namespace detail
}  // namespace cat

#if __has_feature(cxx_exceptions)
#include <cat/detail/cxa_abi.hpp>

#include <cat/runtime>
#include <cat/span>

namespace cat::detail {
namespace {
inline thread_local exception_header* caught_exception = nullptr;
inline thread_local uint4 uncaught_exceptions = 0u;

void
deallocate_exception(exception_header* p_header) {
   allocator_dyn* const p_allocator = exception_allocator();
   if (p_allocator == nullptr) {
      terminate();
   }
   p_allocator->free_multi(
      span<byte>(
         __builtin_bit_cast(byte*, p_header), p_header->allocation_bytes
      )
   );
}

void
destroy_exception(exception_header* p_header) {
   if (p_header->destructor != nullptr) {
      p_header->destructor(p_header + 1);
   }
   deallocate_exception(p_header);
}

void
exception_cleanup(
   unwind_reason_code /*reason*/, unwind_exception* p_exception
) {
   destroy_exception(header_from_unwind(p_exception));
}
}  // namespace
}  // namespace cat::detail

// NOLINTBEGIN(bugprone-std-namespace-modification)
namespace std {
[[noreturn]]
void
terminate() {
   cat::terminate();
}
}  // namespace std

// NOLINTEND(bugprone-std-namespace-modification)

namespace cat::detail {
extern "C" {
// NOLINTNEXTLINE(bugprone-reserved-identifier)
auto
__cxa_allocate_exception(uword bytes) noexcept(false) -> void* {
   if (bytes > __SIZE_MAX__ - sizeof(exception_header)) {
      terminate();
   }

   allocator_dyn* const p_allocator = exception_allocator();
   if (p_allocator == nullptr) {
      terminate();
   }

   // Registered allocator storage includes the exception header.
   constexpr ualign alignment = alignof(exception_header);
   idx const requested =
      narrow_cast<idx>(bytes + sizeof(exception_header)).assert();
   auto const allocation =
      p_allocator->align_salloc_multi<byte>(alignment, requested);
   if (!allocation.has_value()) {
      terminate();
   }

   auto [mapping, allocation_bytes] = allocation.value();
   auto* const p_header = new (mapping.data()) exception_header{};
   p_header->allocation_bytes = allocation_bytes;
   return p_header + 1;
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
void
__cxa_free_exception(void* p_object) {
   deallocate_exception(header_from_user(p_object));
}

[[noreturn]]
// NOLINTNEXTLINE(bugprone-reserved-identifier)
void
__cxa_throw(
   // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
   void* p_object, void const* p_type, void (*p_destructor)(void*)
) {
   auto* const p_header = header_from_user(p_object);
   p_header->type = p_type;
   p_header->destructor = p_destructor;
   p_header->adjusted = p_object;
   p_header->unwind.exception_class = exception_class_primary;
   p_header->unwind.exception_cleanup = exception_cleanup;
   ++uncaught_exceptions;
   auto _ = _Unwind_RaiseException(&p_header->unwind);
   terminate();
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
auto
__cxa_get_exception_ptr(unwind_exception* p_exception) -> void* {
   return header_from_unwind(p_exception)->adjusted;
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
auto
__cxa_begin_catch(unwind_exception* p_exception) -> void* {
   if (!is_native_exception(p_exception->exception_class)) {
      terminate();
   }

   auto* const p_header = header_from_unwind(p_exception);
   p_header->handlers =
      p_header->handlers < 0 ? -p_header->handlers + 1 : p_header->handlers + 1;
   if (caught_exception != p_header) {
      p_header->next = caught_exception;
      caught_exception = p_header;
   }

   --uncaught_exceptions;
   return p_header->adjusted;
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
void
__cxa_end_catch() {
   auto* const p_header = caught_exception;
   if (p_header == nullptr) {
      terminate();
   }

   if (p_header->handlers < 0) {
      if (++p_header->handlers == 0) {
         caught_exception = p_header->next;
      }
   } else if (--p_header->handlers == 0) {
      caught_exception = p_header->next;
      destroy_exception(p_header);
   }
}

[[noreturn]]
// NOLINTNEXTLINE(bugprone-reserved-identifier)
void
__cxa_rethrow() {
   auto* const p_header = caught_exception;
   if (p_header == nullptr) {
      terminate();
   }

   p_header->handlers = -p_header->handlers;
   ++uncaught_exceptions;
   auto _ = _Unwind_RaiseException(&p_header->unwind);
   terminate();
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
auto
__cxa_uncaught_exceptions() -> uint4 {
   return uncaught_exceptions;
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
auto
__cxa_uncaught_exception() -> bool {
   return uncaught_exceptions != 0;
}

[[noreturn]]
// NOLINTNEXTLINE(bugprone-reserved-identifier)
void
__cxa_call_terminate(unwind_exception* /*exception*/) {
   terminate();
}
}

}  // namespace cat::detail

#endif
