#include <cat/format>

namespace cat::detail {
namespace {

template <typename Arg>
auto
format_float_arg(
   allocator_ref<basic_dyn_allocator<dyn_reallocate>> allocator,
   format_contiguous_output_iterator<basic_dyn_allocator<dyn_reallocate>>&
      output,
   format_args const& arguments, idx argument_index
) -> scaredy_format<void> {
   maybe const result =
      to_chars(allocator, arguments.template get<Arg>(argument_index));
   if (!result.has_value()) {
      return format_errors::out_of_memory;
   }
   str_view const new_string = result.value();
   for (idx i; i < new_string.size(); ++i) {
      maybe insert_result = output.insert(allocator, new_string[i]);
      if (!insert_result.has_value()) {
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
