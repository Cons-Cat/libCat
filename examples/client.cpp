#include <cat/array>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto main(int argc, char* p_argv[]) -> int {
    cat::socket_unix<cat::socket_type::stream> socket;
    socket.path_name = cat::fixed_string<108>::padded("\0/tmp/temp.sock");

    socket.create().or_exit();
    socket.connect().or_exit();

    // Send all command line arguments to the server.
    for (int i = 1; i < argc; ++i) {
        socket.send_string(p_argv[i]).or_exit();
        if (i < argc - 1) {
            // TODO: What does this flag `0b1` mean?
            socket.send_string(" ", 0b1).or_exit();
        }
    }

    socket.close().or_exit();
}
