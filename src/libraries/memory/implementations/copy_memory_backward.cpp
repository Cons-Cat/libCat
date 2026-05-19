#include <cat/arithmetic>
#include <cat/memory>

namespace cat::detail {

void
copy_memory_backward_impl(void const* _Nonnull p_source,
                          void* _Nonnull p_destination, idx bytes) {
   // TODO: Provide a high throughput implementation of this function.
   copy_memory_backward_scalar(p_source, p_destination, bytes);
}

}  // namespace cat::detail
