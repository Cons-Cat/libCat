#include <cat/iterable>
#include <cat/meta>
#include <cat/null_allocator>
#include <cat/page_allocator>
#include <cat/runtime>
#include <cat/span>
#include <cat/stacktrace>
#include <cat/thread>
#include <cat/utility>

#include "../unit_tests.hpp"

namespace {

struct capture_pair {
   cat::stacktrace full;
   cat::stacktrace skipped;
};

[[gnu::noinline, clang::disable_tail_calls]]
auto
capture_traces() -> capture_pair {
   cat::nop();
   return {
      .full = cat::stacktrace::current(pager).verify(),
      .skipped = cat::stacktrace::current(pager, 1u).verify(),
   };
}

[[gnu::noinline, clang::disable_tail_calls]]
auto
inner_trace() -> cat::stacktrace {
   cat::nop();
   return cat::stacktrace::current(pager).verify();
}

[[gnu::noinline, clang::disable_tail_calls]]
auto
middle_trace() -> cat::stacktrace {
   cat::nop();
   return inner_trace();
}

[[gnu::noinline, clang::disable_tail_calls]]
auto
outer_trace() -> cat::stacktrace {
   cat::nop();
   return middle_trace();
}

// Recurse `remaining` times before capturing, to produce a trace deeper than
// this `stacktrace`'s inline storage.
// NOLINTNEXTLINE(misc-no-recursion)
[[gnu::noinline, clang::disable_tail_calls]]
auto
deep_trace(idx remaining) -> cat::stacktrace {
   cat::nop();
   if (remaining == 0u) {
      return cat::stacktrace::current(pager).verify();
   }
   return deep_trace(remaining.raw - 1u);
}

// Recurse before capturing through an allocator that may refuse to grow the
// trace.
// NOLINTNEXTLINE(misc-no-recursion)
[[gnu::noinline, clang::disable_tail_calls]]
auto
deep_maybe_trace(cat::dyn_allocator allocator, idx remaining)
   -> cat::maybe<cat::stacktrace> {
   cat::nop();
   if (remaining == 0u) {
      return cat::stacktrace::current(allocator);
   }
   return deep_maybe_trace(allocator, remaining.raw - 1u);
}

}  // namespace

$test(stacktrace_on_child_thread) {
   void* _Nullable p_frame = nullptr;
   void* _Nullable p_stack = nullptr;
   void* _Nullable p_instruction = nullptr;
   idx frame_count = 0u;

   cat::thread worker;
   $defer {
      worker.free(pager);
   };
   worker
      .spawn(
         pager, 16_uki,
         [&] {
            p_frame = __builtin_frame_address(0);
            p_stack = cat::get_stack_pointer();
            cat::stacktrace trace = cat::stacktrace::current(pager).verify();
            frame_count = trace.size();
            if (!trace.is_empty()) {
               p_instruction = trace[0u].native();
            }
         }
      )
      .verify();
   worker.join().verify();

   cat::verify(p_frame != nullptr);
   cat::verify(p_stack != nullptr);
   cat::verify(uintptr<void>(p_frame) > uintptr<void>(p_stack));
   cat::verify(frame_count > 0u);
   cat::verify(p_instruction != nullptr);
}

$test(stacktrace_current_captures_caller) {
   cat::stacktrace const trace = cat::stacktrace::current(pager).verify();
   cat::verify(!trace.is_empty());
   cat::verify(trace.size() <= cat::stacktrace::max_size());
   cat::verify(trace.size() <= trace.capacity());
   cat::verify(bool{trace[0u]});
   cat::verify(trace[0u].native() != nullptr);
   static_assert(
      cat::is_same<decltype(trace[0u]), cat::stacktrace_entry const&>
   );
}

$test(stacktrace_depth_and_skip) {
   cat::stacktrace const none = cat::stacktrace::current(pager, 0u, 0u).verify();
   cat::verify(none.is_empty());
   cat::verify(none.size() == 0u);

   cat::stacktrace const one = cat::stacktrace::current(pager, 0u, 1u).verify();
   cat::verify(one.size() == 1u);
   cat::verify(bool{one[0u]});

   cat::stacktrace const skipped_all =
      cat::stacktrace::current(pager, 1'024u).verify();
   cat::verify(skipped_all.is_empty());

   capture_pair const pair = capture_traces();
   cat::verify(!pair.full.is_empty());
   if (pair.full.size() > 1u) {
      cat::verify(pair.skipped.size() == pair.full.size() - 1u);
      cat::verify(pair.skipped[0u] == pair.full[1u]);
   }
}

// A trace is not capped at its inline capacity. It grows through its
// allocator instead.
$test(stacktrace_grows_past_inline_storage) {
   cat::stacktrace const deep = deep_trace(96u);
   cat::verify(deep.size() > 64u);
   cat::verify(deep.capacity() >= deep.size());
   cat::verify(cat::stacktrace::max_size() > deep.size());

   cat::stacktrace const shallow = deep_trace(0u);
   cat::verify(deep.size() > shallow.size());

   // `max_depth` bounds a deep walk without bounding the type.
   cat::stacktrace const bounded =
      cat::stacktrace::current(pager, 0u, 70u).verify();
   cat::verify(bounded.size() <= 70u);
}

// Capturing reports failure instead of silently truncating, but only once a
// trace outgrows its inline storage.
$test(stacktrace_allocation_failure) {
   cat::null_allocator nothing;
   cat::verify(deep_maybe_trace(nothing, 96u).is_empty());
   cat::verify(cat::stacktrace::current(nothing, 0u, 8u).has_value());
}

$test(stacktrace_entry_and_container) {
   cat::stacktrace_entry const empty{};
   cat::verify(!bool{empty});
   cat::verify(empty.native() == nullptr);

   cat::stacktrace first = cat::stacktrace::current(pager).verify();
   cat::stacktrace second = first.clone().verify();
   cat::verify(first == second);
   static_assert(
      cat::is_same<decltype(first[0u]), cat::stacktrace_entry const&>
   );

   cat::stacktrace other(pager);
   first.swap(other);
   cat::verify(other.size() == second.size());
   cat::verify(first.is_empty());

   idx count = 0u;
   for (cat::stacktrace_entry const& frame : other) {
      cat::verify(bool{frame});
      cat::verify(frame == other[count]);
      count = count + 1u;
   }
   cat::verify(count == other.size());
   if (!other.is_empty()) {
      cat::verify(*other.begin() == other[0u]);
      cat::verify(*(other.end() - 1) == other.back());
   }
}

$test(stacktrace_iteration) {
   static_assert(cat::is_iterable<cat::stacktrace>);
   static_assert(cat::is_random_access_collection<cat::stacktrace>);

   cat::stacktrace trace = cat::stacktrace::current(pager).verify();
   cat::verify(!trace.is_empty());
   cat::verify(cat::read_at(trace, 0u) == trace[0u]);

   cat::stacktrace const& const_trace = trace;
   static_assert(
      cat::is_same<decltype(*const_trace.begin()), cat::stacktrace_entry const&>
   );
   static_assert(
      cat::is_same<decltype(*trace.cbegin()), cat::stacktrace_entry const&>
   );
   static_assert(
      cat::is_random_access_stepanov_iterator<decltype(const_trace.begin())>
   );

   auto first = const_trace.begin();
   auto last = const_trace.end();
   cat::verify(*first == const_trace[0u]);
   cat::verify(*(last - 1) == const_trace.back());
   cat::verify((last - first) == const_trace.size());

   auto cfirst = trace.cbegin();
   auto clast = trace.cend();
   cat::verify(*cfirst == *first);
   cat::verify(*(clast - 1) == *(last - 1));

   idx index = 0u;
   for (cat::stacktrace_entry const& frame : const_trace) {
      cat::verify(frame == const_trace[index]);
      ++index;
   }
   cat::verify(index == const_trace.size());

   index = 0u;
   for (cat::stacktrace_entry const& frame : cat::as_const(trace)) {
      cat::verify(frame == trace[index]);
      ++index;
   }
   cat::verify(index == trace.size());

   auto rfirst = const_trace.rbegin();
   cat::verify(*rfirst == const_trace.back());
   auto crfirst = const_trace.crbegin();
   cat::verify(*crfirst == *rfirst);

   cat::span<cat::stacktrace_entry const> frames(const_trace);
   cat::verify(frames.size() == const_trace.size());

   idx reverse_index = const_trace.size();
   auto reversed = frames | cat::reverse();
   auto reverse_context = cat::iterate(reversed);
   auto const reverse_result =
      reverse_context.run_while([&](cat::stacktrace_entry const& frame) -> bool {
         reverse_index.raw -= 1u;
         cat::verify(frame == const_trace[reverse_index]);
         return true;
      });
   cat::verify(reverse_result == cat::iteration_result::complete);
   cat::verify(reverse_index == 0u);

   if (trace.size() >= 2u) {
      idx index = trace.size();
      idx taken = 0u;
      auto last_two = frames | cat::reverse() | cat::take(2u);
      auto context = cat::iterate(last_two);
      auto const result =
         context.run_while([&](cat::stacktrace_entry const& frame) -> bool {
            index.raw -= 1u;
            cat::verify(frame == trace[index]);
            ++taken;
            return true;
         });
      cat::verify(result == cat::iteration_result::complete);
      cat::verify(taken == 2u);
   }
}
