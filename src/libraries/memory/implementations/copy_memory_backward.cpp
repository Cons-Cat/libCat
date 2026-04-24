#include <cat/arithmetic>
#include <cat/memory>

namespace cat::detail {

void
copy_memory_backward_impl(void const* p_source, void* p_destination,
                          idx bytes) {
   // TODO: Provide a high throughput implementation of this function.
   copy_memory_backward_scalar(p_source, p_destination, bytes);
}

}  // namespace cat::detail
