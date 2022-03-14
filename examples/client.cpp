#include <buffer>
#include <linux>
#include <memory>
#include <socket>
#include <string>

void meow(int argc, char* argv[]) {
    SocketLocal<SocketType::stream> socket;
    char const* socket_name = "/tmp/temp.sock";

    // `1` is a streaming socket.
    FileDescriptor data_socket = nix::create_socket_local(1, 0).or_panic();
    std::copy_memory(socket_name, &socket.path_name,
                     sizeof(socket.path_name) - 1);

    nix::connect_socket(data_socket, &socket, socket.get_size()).or_panic();

    // Send all command line arguments to the server.
    for (isize i = 1; i < argc; i++) {
        nix::send_buffer(data_socket, argv[i], std::string_length(argv[i]), 0)
            .or_panic();
        if (i < argc - 1) {
            nix::send_buffer(data_socket, " ", 1, 0).or_panic();
        }
    }

    nix::close(data_socket).or_panic();
    std::exit();
}
