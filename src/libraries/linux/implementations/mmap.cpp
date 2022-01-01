// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

/* `mmap()` wraps the `mmap` Linux syscall. This returns the virtual
 * memory address which it has allocated a page at. */
auto mmap(usize beginning_address, usize bytes_size, usize protections,
          usize flags, FileDescriptor file_descriptor, usize pages_offset)
    -> Result<void*> {
    return syscall6(9u, beginning_address, bytes_size, protections, flags,
                    file_descriptor, pages_offset * page_size);
}
