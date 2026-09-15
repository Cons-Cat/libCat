#include <cat/array>
#include <cat/format>
#include <cat/iterable>
#include <cat/linear_allocator>
#include <cat/meta>
#include <cat/null_allocator>
#include <cat/page_allocator>
#include <cat/runtime>
#include <cat/span>
#include <cat/stacktrace>
#include <cat/thread>
#include <cat/utility>

#include "../../src/libraries/stacktrace/implementations/dwarf_line.hpp"
#include "../unit_tests.hpp"

struct frame_capture {
   cat::stacktrace trace;
   idx inner_line = 0u;
   idx middle_line = 0u;
   idx outer_line = 0u;
};

// These have C linkage so that their symbols are stable across build modes
// and need no demangling.
extern "C" [[gnu::noinline, clang::disable_tail_calls]]
void
cat_test_capture_one_frame(cat::dyn_allocator allocator, frame_capture& out) {
   out.trace = cat::stacktrace::current(allocator, 0u, 1u).verify();
   out.inner_line = __LINE__ - 1u;
}

extern "C" [[gnu::noinline, clang::disable_tail_calls]]
void
cat_test_frame_inner(cat::dyn_allocator allocator, frame_capture& out) {
   out.trace = cat::stacktrace::current(allocator, 0u, 3u).verify();
   out.inner_line = __LINE__ - 1u;
}

extern "C" [[gnu::noinline, clang::disable_tail_calls]]
void
cat_test_frame_middle(cat::dyn_allocator allocator, frame_capture& out) {
   cat_test_frame_inner(allocator, out);
   out.middle_line = __LINE__ - 1u;
}

extern "C" [[gnu::noinline, clang::disable_tail_calls]]
void
cat_test_frame_outer(cat::dyn_allocator allocator, frame_capture& out) {
   cat_test_frame_middle(allocator, out);
   out.outer_line = __LINE__ - 1u;
}

namespace {

struct capture_pair {
   cat::stacktrace full;
   cat::stacktrace skipped;
};

[[gnu::noinline, clang::disable_tail_calls]]
auto
capture_traces() -> capture_pair {
   return {
      .full = cat::stacktrace::current(pager).verify(),
      .skipped = cat::stacktrace::current(pager, 1u).verify(),
   };
}

[[gnu::noinline, clang::disable_tail_calls]]
auto
inner_trace() -> cat::stacktrace {
   return cat::stacktrace::current(pager).verify();
}

[[gnu::noinline, clang::disable_tail_calls]]
auto
middle_trace() -> cat::stacktrace {
   return inner_trace();
}

[[gnu::noinline, clang::disable_tail_calls]]
auto
outer_trace() -> cat::stacktrace {
   return middle_trace();
}

// Recurse `remaining` times before capturing, to produce a trace deeper than
// this `stacktrace`'s inline storage.
[[gnu::noinline, clang::disable_tail_calls]]
auto
// NOLINTNEXTLINE
deep_trace(idx remaining) -> cat::stacktrace {
   if (remaining == 0u) {
      return cat::stacktrace::current(pager).verify();
   }
   return deep_trace(remaining.raw - 1u);
}

// Recurse before capturing through an allocator that may refuse to grow the
// trace.
[[gnu::noinline, clang::disable_tail_calls]]
auto
// NOLINTNEXTLINE
deep_maybe_trace(cat::dyn_allocator allocator, idx remaining)
   -> cat::maybe<cat::stacktrace> {
   if (remaining == 0u) {
      return cat::stacktrace::current(allocator);
   }
   return deep_maybe_trace(allocator, remaining.raw - 1u);
}

auto
has_prefix(cat::str_view string, cat::str_view prefix) -> bool {
   return string.size() >= prefix.size()
          && string.substring(0u, prefix.size()) == prefix;
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

$test(stacktrace_formatting) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   cat::stacktrace_entry const empty{};
   cat::verify(cat::fmt(allocator, "{}", empty).verify() == "0x0");

   allocator.reset();
   cat::stacktrace const one =
      cat::stacktrace::current(allocator, 0u, 1u).verify();
   cat::verify(one.size() == 1u);
   cat::str_view const entry_string =
      cat::fmt(allocator, "{}", one[0u]).verify();
   cat::verify(
      entry_string == cat::fmt(allocator, "{}", one[0u].native()).verify()
   );

   cat::str_view const header = "Stack trace:\n";
   frame_capture captured{.trace = cat::stacktrace(allocator)};
   cat_test_capture_one_frame(allocator, captured);
   cat::str_view const formatted =
      cat::fmt(allocator, "{}", captured.trace).verify();

   // Release builds carry no DWARF, so the location falls back to
   // `<missing-file>` while the symbol still resolves through `.symtab`.
   bool const has_line_info =
      formatted.find("test_stacktrace.cpp:").has_value();
   cat::str_view const location =
      has_line_info
         ? cat::fmt(allocator, "test_stacktrace.cpp:{}", captured.inner_line)
              .verify()
         : cat::str_view("<missing-file>");
   cat::str_view const expected =
      cat::fmt(
         allocator, "{}#1 {} cat_test_capture_one_frame()\n", header, location
      )
         .verify();
   cat::verify(formatted == expected);

   cat::str_view const contextual =
      cat::fmt(allocator, "{:?}", captured.trace).verify();
   cat::verify(has_prefix(contextual, "Stack trace:\n#1 Object \""));
   cat::verify(contextual.find("\", at 0x").has_value());
   cat::verify(
      contextual.find(", in cat_test_capture_one_frame()\n").has_value()
   );
   if (has_line_info) {
      cat::str_view const source =
         cat::fmt(
            allocator, "Source \"tests/src/test_stacktrace.cpp\", line {}, in ",
            captured.inner_line
         )
            .verify();
      cat::verify(contextual.find(source).has_value());
      cat::verify(contextual
                     .find(
                        "out.trace = cat::stacktrace::current(allocator, 0u, "
                        "1u).verify();"
                     )
                     .has_value());
      cat::verify(contextual.find("   > ").has_value());
   }

   cat::str_view const contextual_entry =
      cat::fmt(allocator, "{:?}", captured.trace[0u]).verify();
   cat::verify(has_prefix(contextual_entry, "#1 Object \""));
   cat::verify(
      contextual_entry.find(", in cat_test_capture_one_frame()\n").has_value()
   );

   cat::str_view const first_frame =
      cat::fmt(allocator, "{}", captured.trace[0u]).verify();
   cat::verify(
      cat::fmt(allocator, "[{}]", captured.trace[0u]).verify()
      == cat::fmt(allocator, "[{}]", first_frame).verify()
   );

   cat::stacktrace const none =
      cat::stacktrace::current(allocator, 0u, 0u).verify();
   cat::verify(cat::fmt(allocator, "{}", none).verify() == header);
}

// Three `noinline` frames are walked, symbolized, and formatted. Capturing
// exactly three keeps the whole string predictable.
$test(stacktrace_formatting_three_frames) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   frame_capture captured{.trace = cat::stacktrace(allocator)};
   cat_test_frame_outer(allocator, captured);
   cat::verify(captured.trace.size() >= 3u);
   cat::verify(captured.trace.size() == 3u);

   cat::str_view const formatted =
      cat::fmt(allocator, "{}", captured.trace).verify();
   bool const has_line_info =
      formatted.find("test_stacktrace.cpp:").has_value();
   auto location = [&](idx line) -> cat::str_view {
      if (!has_line_info) {
         return "<missing-file>";
      }
      return cat::fmt(allocator, "test_stacktrace.cpp:{}", line).verify();
   };

   cat::str_view const expected =
      cat::fmt(
         allocator,
         "Stack trace:\n"
         "#1 {} cat_test_frame_inner()\n"
         "#2 {} cat_test_frame_middle()\n"
         "#3 {} cat_test_frame_outer()\n",
         location(captured.inner_line), location(captured.middle_line),
         location(captured.outer_line)
      )
         .verify();
   cat::verify(formatted == expected);
}

// A trace piped through the take family prints like a whole trace, and what
// remains is renumbered from `#1`.
$test(stacktrace_formatting_dropped_frames) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   frame_capture captured{.trace = cat::stacktrace(allocator)};
   cat_test_frame_outer(allocator, captured);
   cat::verify(captured.trace.size() == 3u);

   cat::span<cat::stacktrace_entry const> frames(captured.trace);
   cat::str_view const formatted =
      cat::fmt(allocator, "{}", frames | cat::drop(1u)).verify();
   bool const has_line_info =
      formatted.find("test_stacktrace.cpp:").has_value();
   auto location = [&](idx line) -> cat::str_view {
      if (!has_line_info) {
         return "<missing-file>";
      }
      return cat::fmt(allocator, "test_stacktrace.cpp:{}", line).verify();
   };

   cat::str_view const expected =
      cat::fmt(
         allocator,
         "Stack trace:\n"
         "#1 {} cat_test_frame_middle()\n"
         "#2 {} cat_test_frame_outer()\n",
         location(captured.middle_line), location(captured.outer_line)
      )
         .verify();
   cat::verify(formatted == expected);

   // Dropping every frame leaves only the header.
   cat::verify(
      cat::fmt(allocator, "{}", frames | cat::drop(3u)).verify()
      == "Stack trace:\n"
   );

   cat::str_view const contextual =
      cat::fmt(allocator, "{:?}", frames | cat::drop(2u)).verify();
   cat::verify(has_prefix(contextual, "Stack trace:\n#1 Object \""));
   cat::verify(contextual.find(", in cat_test_frame_outer()\n").has_value());
   cat::verify(contextual.find("#2 ").is_empty());
}

// The assert handler hides its own frames by recognizing them rather than by
// counting them, because an optimizing build inlines a varying number of them
// away. This exercises that mechanism over a chain of known functions, so it
// holds whether or not the build carries DWARF.
$test(stacktrace_drop_while_frames) {
   cat::span page = pager.alloc_multi<cat::byte>(16_uki).verify();
   $defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   frame_capture captured{.trace = cat::stacktrace(allocator)};
   cat_test_frame_outer(allocator, captured);
   cat::verify(captured.trace.size() == 3u);

   cat::detail::symbolizer* _Nullable const p_symbolizer =
      cat::detail::load_symbolizer();
   $defer {
      cat::detail::unload_symbolizer(p_symbolizer);
   };

   // A frame resolves to the entry point of the function containing it, which
   // is what identifies it without a frame count.
   cat::detail::frame_origin const inner =
      cat::detail::resolve_stacktrace_frame(p_symbolizer, captured.trace[0u]);
   cat::detail::frame_origin const middle =
      cat::detail::resolve_stacktrace_frame(p_symbolizer, captured.trace[1u]);
   cat::detail::frame_origin const outer =
      cat::detail::resolve_stacktrace_frame(p_symbolizer, captured.trace[2u]);
   cat::verify(inner.symbol == "cat_test_frame_inner");
   cat::verify(
      inner.p_function == __builtin_bit_cast(void*, &cat_test_frame_inner)
   );
   cat::verify(
      middle.p_function == __builtin_bit_cast(void*, &cat_test_frame_middle)
   );
   cat::verify(
      outer.p_function == __builtin_bit_cast(void*, &cat_test_frame_outer)
   );
   cat::verify(
      cat::detail::resolve_stacktrace_frame(p_symbolizer, {}).p_function
      == nullptr
   );

   auto belongs_to =
      [&](cat::stacktrace_entry entry, auto... functions) -> bool {
      void* _Nullable p_function = nullptr;
      if (entry == captured.trace[0u]) {
         p_function = inner.p_function;
      } else if (entry == captured.trace[1u]) {
         p_function = middle.p_function;
      } else if (entry == captured.trace[2u]) {
         p_function = outer.p_function;
      } else {
         p_function = cat::detail::resolve_stacktrace_frame(p_symbolizer, entry)
                         .p_function;
      }
      return ((p_function == __builtin_bit_cast(void*, functions)) || ...);
   };

   cat::span<cat::stacktrace_entry const> frames(captured.trace);
   auto format_dropped = [&](auto predicate) -> cat::str_view {
      return cat::fmt(allocator, "{}", frames | cat::drop_while(predicate))
         .verify();
   };
   auto yields_from = [&](auto predicate, idx start) {
      auto remaining = frames | cat::drop_while(predicate);
      idx index = start;
      auto context = cat::iterate(remaining);
      auto const result =
         context.run_while([&](cat::stacktrace_entry const& entry) -> bool {
            cat::verify(index < frames.size());
            cat::verify(entry == frames[index]);
            ++index;
            return true;
         });
      cat::verify(result == cat::iteration_result::complete);
      cat::verify(index == frames.size());
   };

   // A predicate that never holds drops nothing.
   yields_from(
      [](cat::stacktrace_entry) {
         return false;
      },
      0u
   );

   // One leading frame is internal, so `#1` is the frame after it.
   cat::str_view const after_inner =
      format_dropped([&](cat::stacktrace_entry entry) {
         return belongs_to(entry, &cat_test_frame_inner);
      });
   bool const has_line_info =
      after_inner.find("test_stacktrace.cpp:").has_value();
   auto location = [&](idx line) -> cat::str_view {
      if (!has_line_info) {
         return "<missing-file>";
      }
      return cat::fmt(allocator, "test_stacktrace.cpp:{}", line).verify();
   };
   cat::verify(
      after_inner
      == cat::fmt(
            allocator,
            "Stack trace:\n"
            "#1 {} cat_test_frame_middle()\n"
            "#2 {} cat_test_frame_outer()\n",
            location(captured.middle_line), location(captured.outer_line)
      )
            .verify()
   );

   // Two are, so the same predicate shape lands on `#1` anyway.
   cat::verify(
      format_dropped([&](cat::stacktrace_entry entry) {
         return belongs_to(
            entry, &cat_test_frame_inner, &cat_test_frame_middle
         );
      })
      == cat::fmt(
            allocator, "Stack trace:\n#1 {} cat_test_frame_outer()\n",
            location(captured.outer_line)
      )
            .verify()
   );

   // A trace that is internal all the way down honestly prints nothing but
   // its header.
   cat::verify(
      format_dropped([&](cat::stacktrace_entry entry) {
         return belongs_to(
            entry, &cat_test_frame_inner, &cat_test_frame_middle,
            &cat_test_frame_outer
         );
      })
      == "Stack trace:\n"
   );

   // Only leading frames are dropped, so an internal frame further out stays.
   yields_from(
      [&](cat::stacktrace_entry entry) {
         return belongs_to(entry, &cat_test_frame_middle);
      },
      0u
   );
}

$test(stacktrace_dwarf_forms) {
   cat::array<cat::byte, 32u> bytes{};
   cat::span<cat::byte const> const input(bytes);

   struct form_case {
      cat::uint8 form;
      idx size;
   };

   for (form_case const entry : {
           form_case{.form = 0x01u, .size = 8u },
           form_case{.form = 0x03u, .size = 2u },
           form_case{.form = 0x04u, .size = 4u },
           form_case{.form = 0x05u, .size = 2u },
           form_case{.form = 0x06u, .size = 4u },
           form_case{.form = 0x07u, .size = 8u },
           form_case{.form = 0x08u, .size = 1u },
           form_case{.form = 0x09u, .size = 1u },
           form_case{.form = 0x0au, .size = 1u },
           form_case{.form = 0x0bu, .size = 1u },
           form_case{.form = 0x0cu, .size = 1u },
           form_case{.form = 0x0du, .size = 1u },
           form_case{.form = 0x0eu, .size = 4u },
           form_case{.form = 0x0fu, .size = 1u },
           form_case{.form = 0x17u, .size = 4u },
           form_case{.form = 0x18u, .size = 1u },
           form_case{.form = 0x19u, .size = 0u },
           form_case{.form = 0x1au, .size = 1u },
           form_case{.form = 0x1bu, .size = 1u },
           form_case{.form = 0x1du, .size = 4u },
           form_case{.form = 0x1eu, .size = 16u},
           form_case{.form = 0x1fu, .size = 4u },
           form_case{.form = 0x20u, .size = 8u },
           form_case{.form = 0x21u, .size = 0u },
           form_case{.form = 0x22u, .size = 1u },
           form_case{.form = 0x23u, .size = 1u },
           form_case{.form = 0x25u, .size = 1u },
           form_case{.form = 0x26u, .size = 2u },
           form_case{.form = 0x27u, .size = 3u },
           form_case{.form = 0x28u, .size = 4u },
           form_case{.form = 0x29u, .size = 1u },
           form_case{.form = 0x2au, .size = 2u },
           form_case{.form = 0x2bu, .size = 3u },
           form_case{.form = 0x2cu, .size = 4u },
   }) {
      cat::maybe<idx> const size =
         cat::detail::decode_dwarf_form_size(input, entry.form);
      cat::verify(size.has_value());
      cat::verify(size.value() == entry.size);
   }

   cat::verify(
      cat::detail::decode_dwarf_form_size(input, 0x0eu, 8u).verify() == 8u
   );
   cat::verify(
      cat::detail::decode_dwarf_form_size(input, 0x01u, 4u, 4u).verify() == 4u
   );
   cat::verify(cat::detail::decode_dwarf_form_size(input, 0xffu).is_empty());
}

$test(stacktrace_depth_and_skip) {
   cat::stacktrace const none =
      cat::stacktrace::current(pager, 0u, 0u).verify();
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
   auto const reverse_result = reverse_context.run_while(
      [&](cat::stacktrace_entry const& frame) -> bool {
         reverse_index.raw -= 1u;
         cat::verify(frame == const_trace[reverse_index]);
         return true;
      }
   );
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
