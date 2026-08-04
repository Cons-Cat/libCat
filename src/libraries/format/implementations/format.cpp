#include <cat/debug>
#include <cat/format>
#include <cat/insert_iterators>

namespace cat {
namespace detail {
namespace {

auto
parse_format_string(
   str_view const format,
   format_handler<basic_dyn_allocator<dyn_reallocate>>& handler
) -> scaredy_format<void> {
   str_view remainder = format;
   idx current_argument;

   while (remainder.size() > 0) {
      maybe<idx> const maybe_open_brace = remainder.find('{');
      iword const to_open_brace = maybe_open_brace.value_or_niche();
      if (maybe_open_brace.is_empty()) {
         assert(to_open_brace == -1);
      }

      maybe<idx> const maybe_close_brace = remainder.find('}');
      iword const to_close_brace = maybe_close_brace.value_or_niche();
      if (maybe_close_brace.is_empty()) {
         assert(to_close_brace == -1);
      }

      if (to_open_brace == to_close_brace) {
         for (char const& character : remainder) {
            maybe result =
               handler.m_output_iterator.insert(handler.m_allocator, character);
            if (result.is_empty()) {
               return format_errors::out_of_memory;
            }
         }
         break;
      }

      for (idx i; i < to_open_brace; ++i) {
         maybe result =
            handler.m_output_iterator.insert(handler.m_allocator, remainder[i]);
         if (result.is_empty()) {
            return format_errors::out_of_memory;
         }
      }

      scaredy_format<void> parse_result;
      switch (handler.m_arguments.m_p_args_indices[current_argument.raw]
                 .m_discriminant) {
         case format_discriminant::int4_type:
            if (format_arg_int4 == nullptr) {
               return format_errors::cannot_format_argument;
            }
            parse_result = format_arg_int4(
               handler.m_allocator, handler.m_output_iterator,
               handler.m_arguments, current_argument
            );
            break;
         case format_discriminant::int8_type:
            if (format_arg_int8 == nullptr) {
               return format_errors::cannot_format_argument;
            }
            parse_result = format_arg_int8(
               handler.m_allocator, handler.m_output_iterator,
               handler.m_arguments, current_argument
            );
            break;
         case format_discriminant::uint4_type:
            if (format_arg_uint4 == nullptr) {
               return format_errors::cannot_format_argument;
            }
            parse_result = format_arg_uint4(
               handler.m_allocator, handler.m_output_iterator,
               handler.m_arguments, current_argument
            );
            break;
         case format_discriminant::uint8_type:
            if (format_arg_uint8 == nullptr) {
               return format_errors::cannot_format_argument;
            }
            parse_result = format_arg_uint8(
               handler.m_allocator, handler.m_output_iterator,
               handler.m_arguments, current_argument
            );
            break;
         case format_discriminant::float_type:
            if (format_arg_float == nullptr) {
               return format_errors::cannot_format_argument;
            }
            parse_result = format_arg_float(
               handler.m_allocator, handler.m_output_iterator,
               handler.m_arguments, current_argument
            );
            break;
         case format_discriminant::double_type:
            if (format_arg_double == nullptr) {
               return format_errors::cannot_format_argument;
            }
            parse_result = format_arg_double(
               handler.m_allocator, handler.m_output_iterator,
               handler.m_arguments, current_argument
            );
            break;
         default:
            __builtin_unreachable();
      }

      if (parse_result.is_empty()) {
         return parse_result;
      }

      ++current_argument;

      idx const skip = idx(to_close_brace) + 1u;
      char const* const p_tail = remainder.data() + skip.raw;
      idx const tail_size = idx(remainder.size() - skip);
      remainder = str_view(p_tail, tail_size);
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
