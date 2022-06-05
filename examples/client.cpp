#include <cat/array>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

void meow(int argc, char* p_argv[]) {
    cat::SocketUnix<cat::SocketType::stream> socket;
    socket.path_name = "/tmp/temp.sock";

    socket.create().or_panic();
    socket.connect().or_panic();

    // Send all command line arguments to the server.
    for (int i = 1; i < argc; ++i) {
        socket.send_string(p_argv[i], 0).or_panic();
        if (i < argc - 1) {
            socket.send_string(" ", 1).or_panic();
        }
    }

    socket.close().or_panic();
    cat::exit();
}
