#include <cat/format>

namespace cat::detail {
namespace {

template <typename Arg>
auto
format_float_arg(
   allocator_ref<basic_dyn_allocator<dyn_reallocate>> allocator,
   format_contiguous_output_iterator<basic_dyn_allocator<dyn_reallocate>>&
      output,
   format_args const& arguments, idx argument_index,
   format_parse_context& parse_context
) -> scaredy_format<void> {
   formatter<Arg> value_formatter{};
   scaredy_format<format_parse_context::const_iterator> const parse_result =
      value_formatter.parse(parse_context);
   if (parse_result.is_empty()) {
      return parse_result.template error<format_errors>();
   }
   parse_context.advance_to(parse_result.value());

   maybe const result =
      to_chars(allocator, arguments.template get<Arg>(argument_index));
   if (result.is_empty()) {
      return format_errors::out_of_memory;
   }
   str_view const new_string = result.value();
   for (idx i; i < new_string.size(); ++i) {
      maybe insert_result = output.insert(allocator, new_string[i]);
      if (insert_result.is_empty()) {
         return format_errors::out_of_memory;
      }
   }
   return monostate;
}

}  // namespace

format_arg_fn const format_arg_float = &format_float_arg<float>;
format_arg_fn const format_arg_double = &format_float_arg<double>;

void
pull_float_format_support() {
}
}  // namespace cat::detail
