#include <allocators>
#include <linux>

void meow(int, char* p_argv[]) {
    nix::FileDescriptor file =
        nix::open_file(p_argv[1], nix::OpenMode::read_only, nix::OpenFlags{0})
            .or_panic();
    nix::FileStatus status = nix::file_descriptor_status(file).or_panic();
    PageAllocator allocator;
    ssize size = status.blksize;
    MemoryHandle buffer = allocator.malloc(size).or_panic();
    String string =
        String{static_cast<char*>(meta::bit_cast<void*>(buffer)), size};

    nix::read(file, string.p_data(), size).or_panic();
    cat::print(string).or_panic();
    cat::exit();
}
