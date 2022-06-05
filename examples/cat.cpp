#include <cat/allocators>
#include <cat/linux>
#include <cat/math>
#include <cat/optional>

constexpr ssize block_size = 4_ki;

auto get_file_size(nix::FileDescriptor file_descriptor)
    -> cat::Optional<ssize> {
    nix::FileStatus status =
        nix::file_descriptor_status(file_descriptor).or_panic();
    if (status.is_regular()) {
        return status.file_size;
    }
    if (status.is_block_device()) {
        return status.block_size;
    }
    return nullopt;
}

void output_to_console(nix::IoVector const& io_vector) {
    // TODO: Create a mutable string type to prevent this undefined behavior.
    // TODO: Make this buffered output to reduce syscalls.
    cat::Byte const* p_buffer = io_vector.p_data();
    ++p_buffer;
    _ = nix::write(nix::FileDescriptor{1}, cat::bit_cast<char const*>(p_buffer),
                   io_vector.size());
}

void read_and_print_file(char* p_file_name) {
    nix::FileDescriptor file_descriptor =
        nix::open_file(p_file_name, nix::OpenMode::read_only)
            .or_panic(
                // "No such file or directory!"
            );
    ssize file_size = get_file_size(file_descriptor).value();
    ssize bytes_remaining = file_size;
    ssize blocks = file_size / block_size;
    ssize current_block = 0;
    if (file_size % block_size > 0) {
        blocks++;
    }

    cat::PageAllocator allocator;
    cat::Span<nix::IoVector> io_vectors;

    auto io_buffer =
        allocator.malloc<nix::IoVector>(cat::ssizeof(io_vectors) * blocks);
    if (!io_buffer.has_value()) {
        cat::exit(1);
    }
    io_vectors =
        cat::Span<nix::IoVector>{&allocator.get(io_buffer.value()), blocks};

    while (bytes_remaining > 0) {
        ssize current_block_size = cat::min(bytes_remaining, block_size);

        // `buffer` should be 4_ki-aligned.
        // TODO: Create an `AnyPtr` to make `Iovector` take in a `void**`.
        // TODO: Handle allocation failure.
        cat::Optional buffer = allocator.malloc<cat::Byte>(block_size);
        if (!buffer.has_value()) {
            _ = allocator.free(io_buffer.value());
            cat::exit(1);
        }

        io_vectors[current_block] =
            nix::IoVector{&allocator.get(buffer.value()), current_block_size};
        ++current_block;
        bytes_remaining -= current_block_size;
    }

    _ = nix::read_vector(file_descriptor, io_vectors).or_panic();

    for (nix::IoVector const& iov : io_vectors) {
        output_to_console(iov);
    }
    _ = allocator.free(io_buffer.value());
}

void meow(int argc, char* p_argv[]) {
    for (int i = 1; i < argc; ++i) {
        read_and_print_file(p_argv[i]);
    }

    cat::exit();
}
