#include <buffer>
#include <linux>
#include <memory>
#include <socket>
#include <string>

void meow(int argc, char* argv[]) {
    SocketLocal<SocketType::stream> socket;
    char const* socket_name = "/tmp/temp.sock";

    socket.create().or_panic();
    std::copy_memory(socket_name, &socket.path_name,
                     sizeof(socket.path_name) - 1);

    socket.connect().or_panic();

    // Send all command line arguments to the server.
    for (ssize i = 1; i < argc; i++) {
        socket.send_buffer(argv[i], 0).or_panic();
        if (i < argc - 1) {
            socket.send_buffer(" ", 1).or_panic();
        }
    }

    socket.close().or_panic();
    std::exit();
}
