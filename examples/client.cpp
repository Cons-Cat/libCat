#include <buffer>
#include <linux>
#include <memory>
#include <socket.h>
#include <string>

void meow() {
    int4 argc = load_argc();
    char** argv = load_argv();

    SocketLocal socket;
    char const* socket_name = "/tmp/temp.sock";

    // `1` is a streaming socket.
    FileDescriptor data_socket = create_socket_local(1, 0).or_panic();
    std::copy_memory(socket_name, &socket.path_name,
                     sizeof(socket.path_name) - 1);

    connect_socket(data_socket, &socket).or_panic();

    // Send all command line arguments to the server.
    for (isize i = 1; i < argc; i++) {
        send_buffer(data_socket, argv[i], std::string_length(argv[i]), 0)
            .or_panic();
    }

    close(data_socket).or_panic();
    exit();
}
