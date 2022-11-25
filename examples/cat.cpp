#include <cat/linux>
#include <cat/math>
#include <cat/maybe>
#include <cat/page_allocator>

constexpr ssize block_size = 4_ki;

auto get_file_size(nix::FileDescriptor file_descriptor)
    -> cat::Maybe<ssize> {
    nix::FileStatus status = nix::sys_fstat(file_descriptor).or_exit();
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
    cat::Byte const* p_buffer = io_vector.data();
    ++p_buffer;
    _ = nix::sys_write(nix::FileDescriptor{1},
                       cat::bit_cast<char const*>(p_buffer), io_vector.size());
}

void read_and_print_file(char* p_file_name) {
    nix::FileDescriptor file_descriptor =
        nix::sys_open(p_file_name, nix::OpenMode::read_only)
            .or_exit("No such file or directory!", 2);
    ssize file_size = get_file_size(file_descriptor).value();
    ssize bytes_remaining = file_size;
    ssize blocks = file_size / block_size;
    ssize current_block = 0;
    if (file_size % block_size > 0) {
        blocks++;
    }

    cat::PageAllocator allocator;
    cat::Span<nix::IoVector> io_vectors;

    nix::IoVector* p_io_buffer =
        allocator.alloc_multi<nix::IoVector>(blocks).or_exit(
            "Failed to allocate memory!", 3);
    io_vectors = cat::Span<nix::IoVector>{p_io_buffer, blocks};

    while (bytes_remaining > 0) {
        ssize current_block_size = cat::min(bytes_remaining, block_size);

        // `MaybePtr` produces an internal compiler error in GCC 12 here.
        cat::Maybe buffer = allocator.alloc_multi<cat::Byte>(block_size);
        if (!buffer.has_value()) {
            allocator.free_multi(p_io_buffer, io_vectors.size());
            cat::exit(4);
        }

        io_vectors[current_block] =
            nix::IoVector{buffer.p_value(), current_block_size};
        ++current_block;
        bytes_remaining -= current_block_size;
    }

    _ = nix::sys_readv(file_descriptor, io_vectors).or_exit(5);

    for (nix::IoVector const& iov : io_vectors) {
        output_to_console(iov);
    }
    allocator.free_multi(p_io_buffer, io_vectors.size());
}

auto main(int argc, char* p_argv[]) -> int {
    if (argc == 1) {
        _ = cat::eprint("At least one file path must be provided!");
        cat::exit(1);
    }
    for (int i = 1; i < argc; ++i) {
        read_and_print_file(p_argv[i]);
    }
}
