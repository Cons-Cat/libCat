#include <allocators>
#include <linux>
#include <optional>

constexpr ssize block_size = 4_ki;

auto get_file_size(nix::FileDescriptor file_descriptor) -> Optional<ssize> {
    nix::FileStatus status =
        nix::file_descriptor_status(file_descriptor).or_panic();
    if (status.is_regular()) {
        return status.file_size;
    }
    if (status.is_block_device()) {
        return status.block_size;
    }
    return none;
}

// TODO: Is this actually faster than `cat::print()`ing a `String`?
void output_to_console(nix::IoVector const& io_vector) {
    // TODO: Create a mutable string type to prevent this undefined behavior.
    // TODO: Make this buffered output to reduce syscalls.
    char* p_buffer = static_cast<char*>(static_cast<void*>(io_vector.p_data()));
    nix::write(1, p_buffer++, io_vector.size()).discard_result();
}

// TODO: Support `Optional<void>`.
void read_and_print_file(char* p_file_name) {
    nix::FileDescriptor file_descriptor =
        nix::open_file(p_file_name, nix::OpenMode::read_only)
            .or_panic("No such file or directory!");
    ssize file_size = get_file_size(file_descriptor).value();
    ssize bytes_remaining = file_size;
    ssize blocks = file_size / block_size;
    ssize current_block = 0;
    if (file_size % block_size > 0) {
        blocks++;
    }

    PageAllocator allocator;
    Span<nix::IoVector> io_vectors = {
        allocator.malloc(meta::ssizeof<Span<nix::IoVector>>() * blocks)
            .or_panic()
            .as_address(),
        blocks};

    while (bytes_remaining > 0) {
        // TODO: Use `cat::max()`
        ssize current_block_size = bytes_remaining;
        if (current_block_size > block_size) {
            current_block_size = block_size;
        }

        // `p_buffer` should be 4_ki-aligned.
        void* p_buffer = allocator.malloc(block_size).or_panic().as_address();
        io_vectors[current_block] = nix::IoVector{p_buffer, current_block_size};
        current_block++;
        bytes_remaining -= current_block_size;
    }

    nix::read_vector(file_descriptor, io_vectors).or_panic();

    for (nix::IoVector const& iov : io_vectors) {
        output_to_console(iov);
    }
}

void meow(int argc, char* p_argv[]) {
    for (ssize i = 1; i < argc; i++) {
        read_and_print_file(p_argv[i]);
    }

    cat::exit();
}
