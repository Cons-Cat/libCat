#include <cat/allocator_parameters>
#include <cat/format>
#include <cat/maybe>
#include <cat/string>

namespace cat {

template auto
vfmt_to_buffer<allocator_dyn>(
   allocator_ref<allocator_dyn> allocator, detail::format_buffer& buffer,
   str_view format, detail::format_args<allocator_dyn> arguments
) -> scaredy_format<void>;

template auto
vfmt_to_sink<allocator_dyn>(
   allocator_ref<allocator_dyn> allocator, void* _Nonnull p_sink,
   detail::format_flush_fn _Nonnull p_flush, str_view format,
   detail::format_args<allocator_dyn> arguments
) -> scaredy_format<void>;

template auto
vprint_fmt<allocator_dyn>(
   allocator_ref<allocator_dyn> allocator [[clang::lifetimebound]],
   str_view format, detail::format_args<allocator_dyn> arguments
) -> maybe<idx>;

}  // namespace cat
