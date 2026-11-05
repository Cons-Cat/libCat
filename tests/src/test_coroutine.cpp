#include <cat/allocator_inplace>
#include <cat/allocator_linear>
#include <cat/allocator_null>
#include <cat/coroutine>

#include "../unit_tests.hpp"

namespace {
constinit idx frame_allocations = 0u;
constinit idx frame_deallocations = 0u;
constinit idx locals_destroyed = 0u;
constinit void* p_observed_frame = nullptr;

struct counted_frame {
   static auto
   operator new(unsigned long size) -> void* {
      ++frame_allocations;
      return g_tests_alloc.alloc_multi<cat::byte>(size).verify().data();
   }

   static void
   operator delete(void* p_frame, unsigned long size) noexcept {
      ++frame_deallocations;
      g_tests_alloc.free(cat::span(static_cast<cat::byte*>(p_frame), size));
   }

   void
   unhandled_exception() {
      cat::verify(false);
   }
};

struct destructor_counter {
   ~destructor_counter() {
      ++locals_destroyed;
   }
};

// A minimal lazy task which resumes its awaiter by symmetric transfer. When it
// is immediately `co_await`ed, its frame can be elided into the awaiter's.
template <typename T>
struct
   [[clang::coro_return_type, clang::coro_lifetimebound,
     clang::coro_await_elidable]] task {
   struct promise_type : counted_frame {
      T value;
      // A task which nothing awaits returns to `run()` when it finishes.
      cat::coroutine_handle<> awaiter = cat::noop_coroutine();

      auto
      get_return_object() -> task {
         return task(handle::from_promise(*this));
      }

      auto
      initial_suspend() noexcept -> cat::suspend_always {
         return {};
      }

      // Returning the awaiter resumes it in a tail call.
      struct final_awaiter {
         [[nodiscard]]
         auto
         await_ready() const noexcept -> bool {
            return false;
         }

         auto
         await_suspend(cat::coroutine_handle<promise_type> self) noexcept
            -> cat::coroutine_handle<> {
            return self.promise().awaiter;
         }

         void
         await_resume() const noexcept {
         }
      };

      auto
      final_suspend() noexcept -> final_awaiter {
         return {};
      }

      void
      return_value(T returned) {
         value = returned;
      }
   };

   using handle = cat::coroutine_handle<promise_type>;

   explicit task(handle coroutine) : m_handle(coroutine) {
   }

   task(task&& other) : m_handle(other.m_handle) {
      other.m_handle = nullptr;
   }

   ~task() {
      if (m_handle) {
         m_handle.destroy();
      }
   }

   [[nodiscard]]
   auto
   await_ready() const noexcept -> bool {
      return false;
   }

   auto
   await_suspend(cat::coroutine_handle<> awaiter) noexcept -> handle {
      m_handle.promise().awaiter = awaiter;
      return m_handle;
   }

   auto
   await_resume() -> T {
      return m_handle.promise().value;
   }

   // Run a task which nothing awaits to completion.
   auto
   run() -> T {
      m_handle.resume();
      cat::verify(m_handle.done());
      return m_handle.promise().value;
   }

   handle m_handle;
};

auto
leaf(int4 const& value) -> task<int4> {
   co_return value * 2;
}

// Not a coroutine, so it must be a `coro_wrapper` to return a `task`.
[[clang::coro_wrapper]]
auto
leaf_wrapper(int4 const& value) -> task<int4> {
   return leaf(value);
}

[[clang::coro_disable_lifetimebound]]
auto
leaf_unchecked(int4 const& value) -> task<int4> {
   co_return value + 1;
}

auto
sum_elidable(
   [[clang::coro_await_elidable_argument]] task<int4> lhs,
   [[clang::coro_await_elidable_argument]] task<int4> rhs
) -> task<int4> {
   co_return (co_await lhs) + (co_await rhs);
}

auto
sum(task<int4> lhs, task<int4> rhs) -> task<int4> {
   co_return (co_await lhs) + (co_await rhs);
}

// Each `co_await`ed `leaf()` is an immediately awaited coroutine call.
auto
awaits_leaves() -> task<int4> {
   int4 result = co_await leaf(1);
   result += co_await leaf(2);
   co_return result;
}

auto
awaits_elidable_arguments() -> task<int4> {
   co_return co_await sum_elidable(leaf(1), leaf(2));
}

auto
awaits_arguments() -> task<int4> {
   co_return co_await sum(leaf(1), leaf(2));
}

auto
// NOLINTNEXTLINE
recurse(int4 depth) -> task<int4> {
   if (depth == 0) {
      co_return 0;
   }
   co_return 1 + co_await recurse(depth - 1);
}

// A minimal eager coroutine that is only ever destroyed after it completes, so
// its destructor needs no dispatch on its suspend point.
struct [[clang::coro_return_type, clang::coro_only_destroy_when_complete]]
eager {
   struct promise_type : counted_frame {
      int4 value;

      auto
      get_return_object() -> eager {
         return eager(handle::from_promise(*this));
      }

      auto
      initial_suspend() noexcept -> cat::suspend_never {
         return {};
      }

      auto
      final_suspend() noexcept -> cat::suspend_always {
         return {};
      }

      void
      return_value(int4 returned) {
         value = returned;
      }
   };

   using handle = cat::coroutine_handle<promise_type>;

   explicit eager(handle coroutine) : m_handle(coroutine) {
   }

   ~eager() {
      cat::verify(m_handle.done());
      m_handle.destroy();
   }

   handle m_handle;
};

auto
eager_square(int4 value) -> eager {
   destructor_counter counter;
   co_return value* value;
}

// An eager coroutine whose frame comes from `cat::make_coroutine()`.
struct [[clang::coro_return_type]] allocated_eager {
   struct promise_type : cat::allocated_promise {
      int4 value;

      // The coroutine call returns this when its frame cannot be allocated.
      static auto
      get_return_object_on_allocation_failure() -> allocated_eager {
         return allocated_eager(nullptr);
      }

      auto
      get_return_object() -> allocated_eager {
         return allocated_eager(handle::from_promise(*this));
      }

      auto
      initial_suspend() noexcept -> cat::suspend_never {
         return {};
      }

      auto
      final_suspend() noexcept -> cat::suspend_always {
         return {};
      }

      void
      return_value(int4 returned) {
         value = returned;
      }

      void
      unhandled_exception() {
         cat::verify(false);
      }
   };

   using handle = cat::coroutine_handle<promise_type>;

   explicit allocated_eager(handle coroutine) : m_handle(coroutine) {
   }

   allocated_eager(allocated_eager&& other) : m_handle(other.m_handle) {
      other.m_handle = nullptr;
   }

   // False when the frame failed to allocate.
   explicit
   operator bool() const {
      return static_cast<bool>(m_handle);
   }

   ~allocated_eager() {
      if (m_handle) {
         m_handle.destroy();
      }
   }

   handle m_handle;
};

auto
allocated_square(int4 value) -> allocated_eager {
   co_return value* value;
}

// A lazy generator of `int4`, whose frame comes from `cat::make_coroutine()`.
struct [[clang::coro_return_type]] generator {
   struct promise_type : cat::allocated_promise {
      int4 value;

      static auto
      get_return_object_on_allocation_failure() -> generator {
         return generator(nullptr);
      }

      auto
      get_return_object() -> generator {
         return generator(handle::from_promise(*this));
      }

      auto
      initial_suspend() noexcept -> cat::suspend_always {
         return {};
      }

      auto
      final_suspend() noexcept -> cat::suspend_always {
         return {};
      }

      auto
      yield_value(int4 yielded) noexcept -> cat::suspend_always {
         value = yielded;
         return {};
      }

      void
      return_void() {
      }

      void
      unhandled_exception() {
         cat::verify(false);
      }
   };

   using handle = cat::coroutine_handle<promise_type>;

   explicit generator(handle coroutine) : m_handle(coroutine) {
   }

   generator(generator&& other) : m_handle(other.m_handle) {
      other.m_handle = nullptr;
   }

   ~generator() {
      if (m_handle) {
         m_handle.destroy();
      }
   }

   explicit
   operator bool() const {
      return static_cast<bool>(m_handle);
   }

   // Resume until the next `co_yield`, or return false at `co_return`.
   [[nodiscard]]
   auto
   next() const -> bool {
      m_handle.resume();
      return !m_handle.done();
   }

   [[nodiscard]]
   auto
   value() const -> int4 {
      return m_handle.promise().value;
   }

   handle m_handle;
};

auto
count_to(int4 limit) -> generator {
   destructor_counter counter;
   p_observed_frame = __builtin_coro_frame();
   for (int4 i = 1; i <= limit; ++i) {
      co_yield i;
   }
}

// Optimizations may elide a frame whose whole lifetime they can see, so these
// hide the coroutine's destruction from its creation.
template <typename Allocator>
[[gnu::noinline]]
auto
make_square_static(Allocator& allocator, int4 value)
   -> cat::maybe<allocated_eager> {
   return cat::make_coroutine<Allocator>(allocator, allocated_square, value);
}

[[gnu::noinline]]
auto
make_square_dyn(cat::allocator_dyn allocator, int4 value)
   -> cat::maybe<allocated_eager> {
   return cat::make_coroutine(allocator, allocated_square, value);
}
}  // namespace

$test(coroutine_make_coroutine) {
   // The frame comes from the allocator passed to `make_coroutine()`.
   cat::span memory = g_tests_alloc.alloc_multi<cat::byte>(1_uki).verify();
   $defer {
      g_tests_alloc.free(memory);
   };

   cat::is_allocator auto linear = cat::make_allocator_linear(memory);
   {
      allocated_eager square = make_square_static(linear, 7).verify();
      cat::verify(square.m_handle.promise().value == 49);
      cat::verify(linear.bytes_used() > 0u);
   }

   // The `allocator_dyn` overload copies its handle into the frame, so the
   // frame can be freed after that parameter is gone.
   linear.reset();
   {
      allocated_eager square = make_square_dyn(linear, 6).verify();
      cat::verify(square.m_handle.promise().value == 36);
      cat::verify(linear.bytes_used() > 0u);
   }

   // The allocator is only bound while `make_coroutine()` runs.
   cat::verify(cat::detail::current_coro_frame_allocator.p_handle == nullptr);

   // Frames are freed through the same allocator, which ASan checks here.
   {
      allocated_eager square =
         cat::make_coroutine(g_tests_alloc, allocated_square, 8).verify();
      cat::verify(square.m_handle.promise().value == 64);
   }

   // Lambdas are coroutines too.
   {
      allocated_eager incremented = cat::make_coroutine(
                                       g_tests_alloc,
                                       [](int4 value) -> allocated_eager {
                                          co_return value + 1;
                                       },
                                       9
      )
                                       .verify();
      cat::verify(incremented.m_handle.promise().value == 10);
   }

   // Allocation failure returns an empty `maybe` without running the
   // coroutine. These frames must escape so that they cannot be elided.
   cat::allocator_null null;
   cat::verify(!make_square_static(null, 2).has_value());
   cat::verify(!make_square_dyn(null, 2).has_value());
}

$test(coroutine_generator) {
   locals_destroyed = 0u;
   p_observed_frame = nullptr;
   {
      generator counter =
         cat::make_coroutine(g_tests_alloc, count_to, 3).verify();
      // The generator is lazy, so its body has not started.
      cat::verify(p_observed_frame == nullptr);
      for (int4 expected = 1; expected <= 3; ++expected) {
         cat::verify(counter.next());
         cat::verify(counter.value() == expected);
      }
      // A coroutine's own frame is the address of its handle.
      cat::verify(p_observed_frame == counter.m_handle.address());
      cat::verify(!counter.next());
      // Completing the body destroys its locals.
      cat::verify(locals_destroyed == 1u);
   }

   // Destroying a coroutine suspended mid-body destroys its live locals.
   locals_destroyed = 0u;
   {
      generator counter =
         cat::make_coroutine(g_tests_alloc, count_to, 10).verify();
      cat::verify(counter.next());
      cat::verify(locals_destroyed == 0u);
   }
   cat::verify(locals_destroyed == 1u);
}

$test(coroutine_inplace) {
   using inplace_allocator = cat::allocator_inplace<128u>;
   inplace_allocator inplace = cat::make_allocator_inplace<128u>();
   {
      generator counter =
         cat::make_coroutine<inplace_allocator>(inplace, count_to, 3).verify();
      int4 sum = 0;
      while (counter.next()) {
         sum += counter.value();
      }
      cat::verify(sum == 6);
   }
   {
      generator counter = cat::make_coroutine(inplace, count_to, 3).verify();
      int4 sum = 0;
      while (counter.next()) {
         sum += counter.value();
      }
      cat::verify(sum == 6);
   }

   // The one-shot buffer is free again once each frame is destroyed.
   cat::verify(inplace.bytes_used() == 0u);
}

$test(coroutine_handle) {
   cat::coroutine_handle<> null_handle;
   cat::verify(!null_handle);
   cat::verify(null_handle.address() == nullptr);
   cat::verify(null_handle == cat::coroutine_handle<>(nullptr));
   // Handles with different promises compare through `coroutine_handle<>`.
   cat::verify(task<int4>::handle() == generator::handle());

   int4 const four = 4;
   task<int4> coroutine = leaf(four);
   task<int4>::handle handle = coroutine.m_handle;
   cat::verify(static_cast<bool>(handle));
   cat::verify(!handle.done());

   // Round trip the frame through every handle conversion.
   cat::verify(task<int4>::handle::from_address(handle.address()) == handle);
   cat::verify(task<int4>::handle::from_promise(handle.promise()) == handle);
   cat::coroutine_handle<> erased = handle;
   cat::verify(erased.address() == handle.address());
   cat::verify(
      erased == cat::coroutine_handle<>::from_address(handle.address())
   );
   cat::verify((erased <=> null_handle) == cat::strong_ordering::greater);

   // A type-erased handle resumes the same frame.
   erased();
   cat::verify(erased.done());
   cat::verify(handle.promise().value == 8);
}

$test(coroutine_noop) {
   cat::noop_coroutine_handle noop = cat::noop_coroutine();
   cat::verify(static_cast<bool>(noop));
   cat::verify(!noop.done());
   cat::verify(noop.address() != nullptr);
   cat::verify(noop == cat::noop_coroutine());
   cat::verify((noop <=> cat::noop_coroutine()) == cat::strong_ordering::equal);

   [[maybe_unused]]
   cat::noop_coroutine_promise& promise = noop.promise();
   noop();
   noop.resume();
   noop.destroy();

   // The compiler's noop frame is safe to resume and destroy through a
   // type-erased handle.
   cat::coroutine_handle<> erased = noop;
   cat::verify(erased.address() == noop.address());
   cat::verify(!erased.done());
   erased.resume();
   erased.destroy();
   cat::verify(!erased.done());
}

$test(coroutine_task) {
   idx const allocations = frame_allocations;
   idx const deallocations = frame_deallocations;

   cat::verify(leaf(4).run() == 8);
   cat::verify(leaf_wrapper(5).run() == 10);
   cat::verify(awaits_leaves().run() == 6);
   cat::verify(awaits_elidable_arguments().run() == 6);
   cat::verify(awaits_arguments().run() == 6);

   {
      int4 const value = 6;
      task<int4> unchecked = leaf_unchecked(value);
      cat::verify(unchecked.run() == 7);
   }

   // Symmetric transfer resumes each awaiter in a tail call, so deep chains
   // of awaits do not overflow the stack.
   cat::verify(recurse(10'000).run() == 10'000);

   // Optimizations may elide frames, but never leak one.
   cat::verify(
      frame_allocations - allocations == frame_deallocations - deallocations
   );
}

$test(coroutine_only_destroy_when_complete) {
   idx const allocations = frame_allocations;
   idx const deallocations = frame_deallocations;
   locals_destroyed = 0u;
   {
      eager square = eager_square(7);
      cat::verify(square.m_handle.done());
      cat::verify(square.m_handle.promise().value == 49);
      cat::verify(locals_destroyed == 1u);
   }

   cat::verify(
      frame_allocations - allocations == frame_deallocations - deallocations
   );
}
