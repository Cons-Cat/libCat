#include <cat/linux>
#include <cat/math>
#include <cat/maybe>
#include <cat/page_allocator>

using namespace cat::literals;
using namespace cat::integers;

constexpr iword block_size = 4_ki;

auto get_file_size(nix::file_descriptor file_descriptor) -> cat::maybe<iword> {
    nix::file_status status = nix::sys_fstat(file_descriptor).or_exit();
    if (status.is_regular()) {
        return status.file_size;
    }
    if (status.is_block_device()) {
        return status.block_size;
    }
    return nullopt;
}

void output_to_console(nix::io_vector const& io_vector) {
    // TODO: Create a mutable string type to prevent this undefined behavior.
    // TODO: Make this buffered output to reduce syscalls.
    cat::byte const* p_buffer = io_vector.data();
    ++p_buffer;
    _ = nix::sys_write(nix::file_descriptor(1),
                       cat::bit_cast<char const*>(p_buffer), io_vector.size());
}

void read_and_print_file(char* p_file_name) {
    nix::file_descriptor file_descriptor =
        nix::sys_open(p_file_name, nix::open_mode::read_only)
            .or_exit("No such file or directory!", 2);
    iword file_size = get_file_size(file_descriptor).value();
    iword bytes_remaining = file_size;
    iword blocks = file_size / block_size;
    iword current_block = 0;
    if (file_size % block_size > 0) {
        blocks++;
    }

    cat::page_allocator pager;

    cat::span<nix::io_vector> io_vectors =
        pager.alloc_multi<nix::io_vector>(blocks).or_exit(
            "Failed to allocate memory!", 3);
    defer(pager.free_multi(io_vectors.data(), io_vectors.size());)

    while (bytes_remaining > 0) {
        iword current_block_size = cat::min(bytes_remaining, block_size);

        // These pages are freed when iterating through the io vectors later.
        cat::byte* p_buffer = pager.alloc_multi<cat::byte>(block_size)
                                  .or_exit("Failed to allocate memory!", 4)
                                  .data();

        io_vectors[current_block] =
            nix::io_vector(p_buffer, current_block_size);
        ++current_block;
        bytes_remaining -= current_block_size;
    }

    _ = nix::sys_readv(file_descriptor, io_vectors).or_exit(5);

    for (nix::io_vector const& iov : io_vectors) {
        output_to_console(iov);
        pager.free(iov.data());
    }
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
