#include <array>
#include <linux>
#include <memory>
#include <socket>
#include <string>

void meow(int argc, char* argv[]) {
    SocketUnix<SocketType::stream> socket;
    socket.path_name = "/tmp/temp.sock";

    socket.create().or_panic();
    socket.connect().or_panic();

    // Send all command line arguments to the server.
    for (ssize i = 1; i < argc; i++) {
        socket.send_string(argv[i], 0).or_panic();
        if (i < argc - 1) {
            socket.send_string(" ", 1).or_panic();
        }
    }

    socket.close().or_panic();
    std::exit();
}
