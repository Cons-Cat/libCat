#if __has_feature(cxx_exceptions)
#include <cat/allocator_linear>
#include <cat/atomic>
#include <cat/exception>
#include <cat/format>
#include <cat/runtime>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {
struct marker {
   int4 value;
};

struct cleanup {
   int4& count;

   ~cleanup() {
      ++count;
   }
};

struct exception_object {
   int4* destroyed;

   ~exception_object() {
      ++*destroyed;
   }
};

struct base {
   int4 value = 31;
};

struct unrelated {
   int4 padding = 0;
};

// NOLINTNEXTLINE(misc-multiple-inheritance)
struct derived : unrelated,
                 base {};

struct virtual_base {
   int4 value = 47;
};

struct left : virtual virtual_base {};

struct right : virtual virtual_base {};

struct diamond : left,
                 right {};

struct repeated_left : base {};

struct repeated_right : base {};

// NOLINTNEXTLINE(misc-multiple-inheritance)
struct ambiguous : repeated_left,
                   repeated_right {};

struct private_derived : private base {};

[[gnu::noinline]]
void
throw_marker(int4& destroyed) {
   cleanup const guard{destroyed};
   throw marker{42};
}

[[gnu::noinline]]
void
rethrow_marker(int4& destroyed) {
   cleanup const guard{destroyed};
   try {
      throw_marker(destroyed);
   } catch (marker const& value) {
      cat::verify(value.value == 42);
      throw;
   }
}
}  // namespace

$test(exceptions_typed_catch_cleanup_rethrow) {
   int4 destroyed = 0;
   bool caught = false;
   try {
      rethrow_marker(destroyed);
   } catch (int4) {
      cat::verify(false);
   } catch (marker const& value) {
      cat::verify(value.value == 42);
      caught = true;
   }
   cat::verify(caught);
   cat::verify(destroyed == 2);
}

$test(exceptions_nested_catch_and_lifetime) {
   int4 destroyed = 0;
   try {
      throw exception_object{&destroyed};
   } catch (exception_object const& outer) {
      cat::verify(outer.destroyed == &destroyed);
      cat::verify(destroyed == 0);
      try {
         throw 17_i4;
      } catch (int4 value) {
         cat::verify(value == 17);
      }
      cat::verify(destroyed == 0);
   }
   cat::verify(destroyed == 1);
   try {
      throw 1.5;
   } catch (int4) {
      cat::verify(false);
   } catch (...) {
      ++destroyed;
   }
   cat::verify(destroyed == 2);
}

$test(exceptions_public_base_and_pointer_conversion) {
   try {
      throw derived{};
   } catch (base const& value) {
      cat::verify(value.value == 31);
   }
   try {
      throw diamond{};
   } catch (virtual_base const& value) {
      cat::verify(value.value == 47);
   }
   derived object;
   try {
      throw &object;                  // NOLINT
   } catch (base const* p_pointer) {  // NOLINT
      cat::verify(p_pointer == static_cast<base*>(&object));
   }
   try {
      throw &object;                  // NOLINT
   } catch (void const* p_pointer) {  // NOLINT
      cat::verify(p_pointer == &object);
   }
   try {
      throw nullptr;
   } catch (int4* p_pointer) {  // NOLINT
      cat::verify(p_pointer == nullptr);
   }
   int4 const value = 3;
   try {
      throw &value;   // NOLINT
   } catch (int4*) {  // NOLINT
      cat::verify(false);
   } catch (int4 const* p_pointer) {  // NOLINT
      cat::verify(p_pointer == &value);
   }
   try {
      throw ambiguous{};
   } catch (base const&) {
      cat::verify(false);
   } catch (...) {  // NOLINT
   }
   try {
      throw private_derived{};
   } catch (base const&) {
      cat::verify(false);
   } catch (...) {  // NOLINT
   }
}

$test(exceptions_parallel_throws) {
   cat::atomic<int4> passed{0};
   auto const worker = [&] {
      for (idx i = 0u; i < 100u; ++i) {
         try {
            throw i;  // NOLINT
         } catch (idx value) {
            if (value == i) {
               ++passed;
            }
         }
      }
   };
   cat::thread first;
   first.spawn(g_tests_alloc, 256_uki, worker).verify();
   cat::thread second;
   second.spawn(g_tests_alloc, 256_uki, worker).verify();
   worker();
   first.join().verify();
   second.join().verify();
   first.free(g_tests_alloc);
   second.free(g_tests_alloc);
   cat::verify(passed.relaxed() == 300);
}

namespace {
[[gnu::noinline]]
void
uncaught_throw() {
   throw marker{1};
}

[[gnu::noinline]]
void
// NOLINTNEXTLINE(bugprone-exception-escape)
noexcept_throw() noexcept {
   uncaught_throw();
}

void
check_termination(void (*p_action)()) {
   auto const child = nix::syscall_volatile<long>(57).verify();
   if (child == 0) {
      p_action();
      cat::exit(0);
   }
   int4 status = 0;
   auto const waited =
      nix::syscall_volatile<long>(61, child, &status, 0, nullptr).verify();
   cat::verify(waited == child);
   auto const bits = uint4(status);
   cat::verify((bits & 0x7fu) == 0u);
   cat::verify((bits >> 8u) == 134u);
}
}  // namespace

$test(exceptions_terminate_uncaught_and_noexcept) {
   check_termination(uncaught_throw);
   check_termination(noexcept_throw);
}

namespace {
struct freestanding_error {
   cat::source_location m_location = cat::source_location::current();

   [[nodiscard]]
   auto
   what() const noexcept -> cat::str_view {
      return cat::str_view("freestanding failure");
   }

   [[nodiscard]]
   auto
   location() const noexcept -> cat::source_location {
      return m_location;
   }
};
}  // namespace

$test(exceptions_cat_exception_message_location_format) {
   cat::source_location const expected = cat::source_location::current();
   cat::exception const original(expected);
   cat::exception const copy = original;
   cat::verify(original.what() == "exception");
   cat::verify(copy.what() == "exception");
   // The viewed range ends on 'e', so this message has no null terminator.
   cat::str_view const sliced = cat::str_view("note-tail").substring(0u, 4u);
   cat::exception const named(sliced);
   cat::exception const named_copy = named;
   cat::verify(named.what() == "note");
   cat::verify(named_copy.what() == "note");
   cat::verify(original.location().line() == expected.line());
   cat::verify(copy.location().line() == expected.line());
   cat::verify(
      cat::str_view(original.location().file_name())
      == cat::str_view(expected.file_name())
   );
   cat::verify(
      cat::str_view(copy.location().file_name())
      == cat::str_view(expected.file_name())
   );

   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(4_uki).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   auto allocator = cat::make_allocator_linear(page);
   cat::verify(cat::fmt(allocator, "{}", original).verify() == "exception");
   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{:?}", original).verify() == R"("exception")"
   );
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{}", named).verify() == "note");
   allocator.reset();
   cat::verify(cat::fmt(allocator, "{:?}", named).verify() == R"("note")");
   allocator.reset();

   cat::allocator_dyn dyn_allocator = g_tests_alloc;
   freestanding_error const error;
   cat::exception_dyn const erased{dyn_allocator, error};
   cat::exception_dyn const erased_copy = erased;
   cat::verify(
      cat::dyn_invoke<cat::dyn_exception_what>(erased) == "freestanding failure"
   );
   cat::verify(
      cat::dyn_invoke<cat::dyn_exception_what>(erased_copy)
      == "freestanding failure"
   );
   cat::verify(
      cat::dyn_invoke<cat::dyn_exception_location>(erased_copy).line()
      == error.location().line()
   );
   cat::verify(
      cat::fmt(allocator, "{}", erased_copy).verify() == "freestanding failure"
   );
   allocator.reset();
   cat::verify(
      cat::fmt(allocator, "{:?}", erased).verify()
      == R"("freestanding failure")"
   );
   allocator.reset();

   bool caught = false;
   try {
      throw erased;
   } catch (cat::exception_dyn const& thrown) {
      cat::verify(
         cat::dyn_invoke<cat::dyn_exception_what>(thrown)
         == "freestanding failure"
      );
      caught = true;
   }
   cat::verify(caught);
}

$test(exceptions_null_and_nested_pointer_matching) {
   try {
      throw static_cast<ambiguous*>(nullptr);  // NOLINT
   } catch (base*) {                           // NOLINT
      cat::verify(false);
   } catch (...) {  // NOLINT
   }
   try {
      throw static_cast<diamond*>(nullptr);  // NOLINT
   } catch (virtual_base* p_pointer) {       // NOLINT
      cat::verify(p_pointer == nullptr);
   }
   int4* p_pointer = nullptr;  // NOLINT(misc-const-correctness)
   try {
      throw &p_pointer;      // NOLINT
   } catch (int4 const**) {  // NOLINT
      cat::verify(false);
   } catch (int4* const* p_value) {  // NOLINT
      cat::verify(p_value == &p_pointer);
   }
   try {
      throw &p_pointer;                    // NOLINT
   } catch (int4 const* const* p_value) {  // NOLINT
      cat::verify(p_value == &p_pointer);
   }
}

namespace {
struct copy_stats {
   int4& copies;
   int4& destroyed;
};

struct copied_exception {
   int4* copies;
   int4* destroyed;

   explicit copied_exception(copy_stats stats)
       : copies(&stats.copies), destroyed(&stats.destroyed) {
   }

   copied_exception(copied_exception const& other)
       : copies(other.copies), destroyed(other.destroyed) {
      ++*copies;
   }

   ~copied_exception() {
      ++*destroyed;
   }
};

struct failing_copy {
   failing_copy() = default;

   failing_copy([[maybe_unused]]
                failing_copy const& other) {
      throw 19_i4;
   }
};
}  // namespace

$test(exceptions_catch_by_value_and_failed_construction) {
   int4 copies = 0;
   int4 destroyed = 0;
   try {
      throw copied_exception{
         copy_stats{
                    .copies = copies,
                    .destroyed = destroyed,
                    }
      };
   } catch (copied_exception value) {  // NOLINT
      cat::verify(value.copies == &copies);
      cat::verify(copies == 1);
      cat::verify(destroyed == 0);
   }
   cat::verify(destroyed == 2);
   try {
      failing_copy const object;
      // NOLINTNEXTLINE
      throw object;
   } catch (int4 value) {
      cat::verify(value == 19);
   }
}

#endif
