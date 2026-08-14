#include <cat/debug>
#include <cat/format>
#include <cat/insert_iterators>

namespace cat {
namespace detail {
namespace {

// Bulk-copy a run of literal text into the output, like `{fmt}`'s `on_text`.
auto
append_literal(
   format_handler<basic_dyn_allocator<dyn_reallocate>>& handler, str_view run
) -> scaredy_format<void> {
   maybe result =
      get_container(handler.m_output_iterator).append(handler.m_allocator, run);
   if (result.is_empty()) {
      return format_errors::out_of_memory;
   }
   return monostate;
}

auto
parse_format_string(
   str_view const format,
   format_handler<basic_dyn_allocator<dyn_reallocate>>& handler
) -> scaredy_format<void> {
   idx current_argument;
   idx index;
   // The start of the pending run of literal text yet to be flushed.
   idx literal_start;

   // Flush literal text in `[literal_start, end_index)` as one copy.
   auto flush = [&](idx end_index) -> scaredy_format<void> {
      if (end_index > literal_start) {
         return append_literal(
            handler,
            str_view(
               format.data() + literal_start.raw, idx(end_index - literal_start)
            )
         );
      }
      return monostate;
   };

   while (index < format.size()) {
      char const character = format[index];

      if (character == '{') {
         if (index + 1u < format.size() && format[index + 1u] == '{') {
            // Keep the first brace in the literal run, drop the second.
            scaredy_format<void> const escaped = flush(index + 1u);
            if (escaped.is_empty()) {
               return escaped;
            }
            index += 2u;
            literal_start = index;
            continue;
         }

         scaredy_format<void> flushed = flush(index);
         if (flushed.is_empty()) {
            return flushed;
         }

         idx const field_start = index + 1u;
         idx close_index = field_start;
         bool found_close = false;
         while (close_index < format.size()) {
            if (format[close_index] == '{') {
               return format_errors::unmatched_open_brace;
            }
            if (format[close_index] == '}') {
               found_close = true;
               break;
            }
            ++close_index;
         }
         if (!found_close) {
            return format_errors::unmatched_open_brace;
         }

         idx cursor = field_start;
         if (cursor < close_index) {
            char const head = format[cursor];
            if (head >= '0' && head <= '9') {
               return format_errors::unknown_format_specifier;
            }
         }

         idx spec_begin = cursor;
         if (cursor < close_index && format[cursor] == ':') {
            spec_begin = cursor + 1u;
         } else if (cursor != close_index) {
            return format_errors::unknown_format_specifier;
         }

         if (current_argument >= handler.m_arguments.m_argument_count) {
            return format_errors::argument_type_mismatch;
         }

         str_view const parse_view(
            format.data() + spec_begin.raw, idx(format.size() - spec_begin)
         );
         format_parse_context parse_context(
            parse_view, handler.m_arguments.m_argument_count
         );

         scaredy_format<void> format_result =
            handler.m_arguments.m_p_formatters[current_argument.raw](
               handler.m_allocator, handler.m_output_iterator,
               handler.m_arguments, current_argument, parse_context
            );
         if (format_result.is_empty()) {
            return format_result;
         }
         if (
            parse_context.begin() == parse_context.end()
            || *parse_context.begin() != '}'
         ) {
            return format_errors::unmatched_open_brace;
         }

         ++current_argument;
         index = close_index + 1u;
         literal_start = index;
         continue;
      }

      if (character == '}') {
         if (index + 1u < format.size() && format[index + 1u] == '}') {
            // Keep the first brace in the literal run, drop the second.
            scaredy_format<void> const escaped = flush(index + 1u);
            if (escaped.is_empty()) {
               return escaped;
            }
            index += 2u;
            literal_start = index;
            continue;
         }
         return format_errors::unmatched_open_brace;
      }

      ++index;
   }

   scaredy_format<void> trailing = flush(index);
   if (trailing.is_empty()) {
      return trailing;
   }

   if (current_argument != handler.m_arguments.m_argument_count) {
      return format_errors::argument_type_mismatch;
   }
   return monostate;
}

}  // namespace

[[gnu::weak]]
format_arg_fn const format_arg_int4 = nullptr;
[[gnu::weak]]
format_arg_fn const format_arg_int8 = nullptr;
[[gnu::weak]]
format_arg_fn const format_arg_uint4 = nullptr;
[[gnu::weak]]
format_arg_fn const format_arg_uint8 = nullptr;
[[gnu::weak]]
format_arg_fn const format_arg_float = nullptr;
[[gnu::weak]]
format_arg_fn const format_arg_double = nullptr;
}  // namespace detail

auto
vfmt(
   basic_dyn_allocator<dyn_reallocate> allocator, str_view const format,
   detail::format_args arguments
) -> scaredy_format<str_view> {
   idx const initial_size = format.size() + 10u;

   maybe maybe_memory = allocator.alloc_multi_uninit<char>(initial_size);
   if (maybe_memory.is_empty()) {
      return format_errors::out_of_memory;
   }

   char* p_memory = maybe_memory.value().data();
   detail::format_buffer<basic_dyn_allocator<dyn_reallocate>> buffer = {
      allocator_ref(allocator), p_memory, 0u, initial_size
   };
   detail::format_handler<basic_dyn_allocator<dyn_reallocate>> handler = {
      allocator, as_back_inserter(buffer), format, arguments
   };

   scaredy_format<void> result = detail::parse_format_string(format, handler);
   if (result.is_empty()) {
      allocator.free(span(buffer.data(), buffer.capacity()));
      return result.error<format_errors>();
   }

   return str_view(buffer.data(), buffer.size());
}

auto
vfmt_to_sink(
   basic_dyn_allocator<dyn_reallocate> allocator, void* _Nonnull p_sink,
   detail::format_flush_fn _Nonnull p_flush, str_view const format,
   detail::format_args arguments
) -> scaredy_format<void> {
   // Stage into stack storage and drain into the sink, so formatting never
   // allocates a copy of the whole result.
   char storage[detail::format_sink_buffer_size.raw];
   detail::format_buffer<basic_dyn_allocator<dyn_reallocate>> buffer = {
      allocator_ref(allocator), storage, detail::format_sink_buffer_size,
      p_sink, p_flush
   };
   detail::format_handler<basic_dyn_allocator<dyn_reallocate>> handler = {
      allocator, as_back_inserter(buffer), format, arguments
   };

   scaredy_format<void> result = detail::parse_format_string(format, handler);
   if (result.is_empty()) {
      return result;
   }
   if (buffer.flush().is_empty()) {
      return format_errors::out_of_memory;
   }
   return monostate;
}

auto
vprint_fmt(
   basic_dyn_allocator<dyn_reallocate> allocator, str_view const format,
   detail::format_args arguments
) -> maybe<idx> {
   str_view const string =
      $prop_as(vfmt(allocator, format, arguments), nullopt);
   maybe const result = print(string);
   allocator.free(string);
   return result;
}

}  // namespace cat
