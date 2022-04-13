#include <allocators>
#include <linux>

void meow(int, char* p_argv[]) {
    nix::FileDescriptor file =
        nix::open_file(p_argv[1], nix::OpenMode::read_only, nix::OpenFlags{0})
            .or_panic("No such file or directory!");
    nix::FileStatus status = nix::file_descriptor_status(file).or_panic(
        "Failed to get file status!");
    PageAllocator allocator;
    ssize size = status.block_size;
    char* p_buffer = static_cast<char*>(static_cast<void*>(
        allocator.malloc(size).or_panic("Failed to allocate memory!")));
    String string = String{p_buffer, size};

    nix::read(file, string.p_data(), size)
        .or_panic("Failed to read from the open file!");
    cat::print(string).or_panic();
    cat::exit();
}
