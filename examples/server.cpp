#include <cat/array>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

auto main() -> int {
    cat::socket_unix<cat::socket_type::stream> listening_socket;
    // A leading null byte puts this path in the abstract namespace.
    listening_socket.path_name =
        cat::fixed_string<108>::padded("\0/tmp/temp.sock");
    listening_socket.create().or_exit();
    listening_socket.bind().or_exit();
    listening_socket.listen(20).or_exit();

    cat::socket_unix<cat::socket_type::stream> recieving_socket;
    cat::fixed_string<12> message_buffer;

    bool exit = false;
    while (!exit) {
        recieving_socket.accept(listening_socket).or_exit();

        while (true) {
            cat::iword message_length =
                recieving_socket
                    .recieve(message_buffer.data(), message_buffer.size())
                    .or_exit();

            cat::string input = {message_buffer.data(), message_buffer.size()};

            // TODO: This comparison is always false.
            if (cat::compare_strings(input, "exit")) {
                _ = cat::println("Exiting.");
                exit = true;
                break;
            }

            if (!cat::compare_strings(input, "")) {
                // Zero out the message buffer's ending.
                for (cat::iword i = message_length; i < input.size(); ++i) {
                    message_buffer[i] = '\0';
                }

                _ = cat::print("Recieved: ");
                _ = cat::println(message_buffer.data());
                break;
            }
        }
    }

    recieving_socket.close().or_exit();
    listening_socket.close().or_exit();
    _ = nix::sys_unlink(listening_socket.path_name.data()).or_exit();
}
