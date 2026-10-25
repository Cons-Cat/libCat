#if __has_feature(cxx_exceptions)
#include <cat/atomic>
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
   // ASan's `throw` hook calls pthread TLS. cat::thread does not provide that.
   // TODO: Make `cat::thread` transparently become a pthread when ASan is
   // enabled.
   auto const worker = [&] [[gnu::no_sanitize_address]]
   {
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
