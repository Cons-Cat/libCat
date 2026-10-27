#if __has_feature(cxx_exceptions)
#include <cat/allocator_linear>
#include <cat/array>
#include <cat/atomic>
#include <cat/exception>
#include <cat/format>
#include <cat/forward_list>
#include <cat/list>
#include <cat/map>
#include <cat/runtime>
#include <cat/thread>
#include <cat/vec>

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

namespace {
// Thrown by `fragile` after `fragile::arm()`.
struct injected_failure {};

// Copying and moving count as constructors. `arm(n)` makes the nth later
// constructor throw, and `live` counts objects whose constructors finished.
struct fragile {
   static inline cat::iword live = 0;
   static inline cat::iword until_throw = 0;
   cat::int4 value;

   static void
   arm(cat::iword count) {
      until_throw = count;
   }

   static void
   disarm() {
      until_throw = 0;
   }

   explicit constexpr fragile(cat::int4 input) : value(input) {
      constructed();
   }

   constexpr fragile() : value(0) {
      constructed();
   }

   constexpr fragile(fragile const& other) : value(other.value) {
      constructed();
   }

   constexpr fragile(fragile&& other) : value(other.value) {
      constructed();
   }

   constexpr auto
   operator=(fragile const& other) -> fragile& {
      value = other.value;
      return *this;
   }

   constexpr auto
   operator=(fragile&& other) -> fragile& {
      value = other.value;
      return *this;
   }

   constexpr ~fragile() {
      --live;
   }

 private:
   static constexpr void
   constructed() {
      if (until_throw > 0) {
         --until_throw;
         if (until_throw == 0) {
            throw injected_failure{};
         }
      }
      ++live;
   }
};

template <typename Fn>
auto
throws_injected_failure(Fn&& fn) -> bool {
   try {
      fn();
   } catch (injected_failure const&) {
      return true;
   }
   return false;
}

template <typename Vector>
void
fill_vec(Vector& vector, int4 count) {
   for (int4 i = 0; i < count; ++i) {
      cat::verify(vector.emplace_back(g_tests_alloc, i).has_value());
   }
}

// Fill a `vec` with `0, 1, 2...` until it has no spare capacity.
template <typename Vector>
auto
fill_vec_to_capacity(Vector& vector) -> int4 {
   fill_vec(vector, 1);
   while (vector.size() < vector.capacity()) {
      cat::verify(
         vector.emplace_back(g_tests_alloc, int4(vector.size().raw)).has_value()
      );
   }
   return int4(vector.size().raw);
}

template <typename Vector>
void
verify_vec_sequence(Vector const& vector, int4 count) {
   cat::verify(vector.size() == idx(count));
   for (int4 i = 0; i < count; ++i) {
      cat::verify(vector[idx(i)].value == i);
   }
}

using allocator_type = __typeof_unqual__(g_tests_alloc);
}  // namespace

// Reallocation copies throwing-move elements, so a throwing copy leaves the
// original elements untouched.
$test(vec_reallocation_strong_guarantee) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   int4 const count = fill_vec_to_capacity(vector);
   idx const capacity = vector.capacity();
   cat::verify(vector.size() == capacity);

   fragile::arm(2);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.emplace_back(g_tests_alloc, count);
   }));
   verify_vec_sequence(vector, count);
   cat::verify(vector.capacity() == capacity);
   cat::verify(fragile::live == iword(count));

   vector.free(g_tests_alloc);
   cat::verify(fragile::live == 0);
}

$test(vec_inline_reallocation_strong_guarantee) {
   fragile::live = 0;
   cat::vec<fragile> vector;
   fill_vec(vector, 4);

   fragile::arm(3);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.reserve(g_tests_alloc, 16u);
   }));
   verify_vec_sequence(vector, 4);
   cat::verify(fragile::live == 4);

   vector.free(g_tests_alloc);
   cat::verify(fragile::live == 0);
}

$test(vec_emplace_back_throwing_constructor) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   fill_vec(vector, 2);
   fragile const source(7);

   fragile::arm(1);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.push_back(g_tests_alloc, source);
   }));
   verify_vec_sequence(vector, 2);
   cat::verify(fragile::live == 3);

   vector.free(g_tests_alloc);
}

$test(vec_resize_strong_guarantee) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   fill_vec(vector, 3);
   cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());
   fragile const source(9);

   fragile::arm(4);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.resize(g_tests_alloc, 10u, source);
   }));
   verify_vec_sequence(vector, 3);
   cat::verify(fragile::live == 4);

   vector.free(g_tests_alloc);
}

$test(vec_clone_throwing_copy) {
   fragile::live = 0;
   cat::vec_big<fragile> vector;
   fill_vec(vector, 6);

   fragile::arm(4);
   cat::verify(throws_injected_failure([&] {
      auto _ = vector.clone(g_tests_alloc);
   }));
   cat::verify(fragile::live == 6);

   vector.free(g_tests_alloc);
}

// Insertion has the basic guarantee: every element that is not destroyed is
// still owned by the `vec`.
$test(vec_insert_range_basic_guarantee) {
   fragile::live = 0;
   cat::array<fragile, 3u> const range = {
      fragile(100), fragile(101), fragile(102)
   };

   for (iword failure = 1; failure <= 8; ++failure) {
      cat::vec_big<fragile> vector;
      fill_vec(vector, 5);
      cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());

      fragile::arm(failure);
      bool const threw = throws_injected_failure([&] {
         auto _ = vector.insert_range(g_tests_alloc, 1u, range);
      });
      fragile::disarm();
      cat::verify(fragile::live == iword(vector.size()) + 3);
      if (!threw) {
         cat::verify(vector.size() == 8u);
      }

      vector.free(g_tests_alloc);
      cat::verify(fragile::live == 3);
   }
}

$test(vec_replace_with_range_basic_guarantee) {
   fragile::live = 0;
   cat::array<fragile, 4u> const range = {
      fragile(100), fragile(101), fragile(102), fragile(103)
   };

   for (iword failure = 1; failure <= 8; ++failure) {
      cat::vec_big<fragile> vector;
      fill_vec(vector, 5);
      cat::verify(vector.reserve(g_tests_alloc, 16u).has_value());

      fragile::arm(failure);
      throws_injected_failure([&] {
         auto _ = vector.replace_with_range(g_tests_alloc, 1u, 2u, range);
      });
      fragile::disarm();
      cat::verify(fragile::live == iword(vector.size()) + 4);

      vector.free(g_tests_alloc);
      cat::verify(fragile::live == 4);
   }
}

// Inserting into a full `vec` reallocates around the new elements, so a throw
// anywhere leaks nothing, and succeeding preserves element order.
$test(vec_insert_range_reallocating) {
   fragile::live = 0;
   cat::array<fragile, 3u> const range = {
      fragile(100), fragile(101), fragile(102)
   };

   for (idx position = 0u; position <= 4u; ++position) {
      for (iword failure = 1; failure <= 10; ++failure) {
         cat::vec_big<fragile> vector;
         idx const old_size = idx(fill_vec_to_capacity(vector));

         fragile::arm(failure);
         bool const threw = throws_injected_failure([&] {
            auto _ = vector.insert_range(g_tests_alloc, position, range);
         });
         fragile::disarm();
         cat::verify(fragile::live == iword(vector.size()) + 3);
         if (!threw) {
            cat::verify(vector.size() == old_size + 3u);
            for (idx i; i < vector.size(); ++i) {
               int4 const expected = i < position ? int4(i.raw)
                                     : i < position + 3u
                                        ? int4(100 + (i - position).raw)
                                        : int4((i - 3u).raw);
               cat::verify(vector[i].value == expected);
            }
         }
         vector.free(g_tests_alloc);
         cat::verify(fragile::live == 3);
      }
   }
}

// Freeing a list destroys each element exactly once.
$test(list_element_lifetimes) {
   fragile::live = 0;
   cat::list<fragile> list;
   for (int4 i = 0; i < 4; ++i) {
      cat::verify(
         list.emplace_back<allocator_type>(g_tests_alloc, i).has_value()
      );
   }
   list.erase<allocator_type>(g_tests_alloc, list.begin());
   list.pop_front<allocator_type>(g_tests_alloc);
   cat::verify(fragile::live == 2);
   list.free<allocator_type>(g_tests_alloc);
   cat::verify(fragile::live == 0);
}

$test(list_throwing_insert) {
   fragile::live = 0;
   fragile const source(7);
   cat::list<fragile> list;
   cat::verify(
      list.push_back<allocator_type>(g_tests_alloc, source).has_value()
   );

   fragile::arm(1);
   cat::verify(throws_injected_failure([&] {
      auto _ = list.push_back<allocator_type>(g_tests_alloc, source);
   }));
   fragile::arm(1);
   cat::verify(throws_injected_failure([&] {
      auto _ = list.emplace_front<allocator_type>(g_tests_alloc, source);
   }));
   cat::verify(list.size() == 1u);
   cat::verify(fragile::live == 2);

   fragile::arm(1);
   cat::verify(throws_injected_failure([&] {
      auto _ = list.clone<allocator_type>(g_tests_alloc);
   }));
   cat::verify(fragile::live == 2);

   list.free<allocator_type>(g_tests_alloc);
   cat::verify(fragile::live == 1);
}

$test(forward_list_throwing_insert) {
   fragile::live = 0;
   fragile const source(7);
   cat::forward_list<fragile> list;
   cat::verify(
      list.push_front<allocator_type>(g_tests_alloc, source).has_value()
   );

   fragile::arm(1);
   cat::verify(throws_injected_failure([&] {
      auto _ = list.push_front<allocator_type>(g_tests_alloc, source);
   }));
   cat::verify(list.size() == 1u);
   cat::verify(fragile::live == 2);

   list.free<allocator_type>(g_tests_alloc);
   cat::verify(fragile::live == 1);
}

// If moving entries throws while a `map` grows, the `map` is cleared and
// leaks nothing.
$test(map_throwing_rehash) {
   for (iword failure = 1; failure <= 64; failure += 3) {
      fragile::live = 0;
      cat::map_big<int4, fragile> map;
      for (int4 i = 0; i < 20; ++i) {
         cat::verify(
            map.insert<allocator_type>(g_tests_alloc, i, fragile(i)).has_value()
         );
      }

      fragile::arm(failure);
      throws_injected_failure([&] {
         for (int4 i = 20; i < 200; ++i) {
            auto _ = map.insert<allocator_type>(g_tests_alloc, i, fragile(i));
         }
      });
      fragile::disarm();
      cat::verify(fragile::live == iword(map.size()));

      map.free<allocator_type>(g_tests_alloc);
      cat::verify(fragile::live == 0);
   }
}

#endif
