#include <cat/array>
#include <cat/linux>
#include <cat/memory>
#include <cat/socket>
#include <cat/string>

void meow() {
    cat::SocketUnix<cat::SocketType::stream> listening_socket;
    // A leading null byte puts this path in the abstract namespace.
    listening_socket.path_name =
        cat::StaticString<108>::padded("\0/tmp/temp.sock");
    listening_socket.create().or_panic();
    listening_socket.bind().or_panic();
    listening_socket.listen(20).or_panic();

    cat::SocketUnix<cat::SocketType::stream> recieving_socket;
    cat::StaticString<12> message_buffer;

    bool1 exit = false;
    while (!exit) {
        recieving_socket.accept(listening_socket).or_panic();

        while (true) {
            ssize message_length =
                recieving_socket
                    .recieve(message_buffer.p_data(), message_buffer.size())
                    .or_panic();

            cat::String input = {message_buffer.p_data(),
                                 message_buffer.size()};

            // TODO: This comparison is always false.
            if (cat::compare_strings(input, "exit")) {
                _ = cat::print_line("Exiting.");
                exit = true;
                break;
            }

            if (!cat::compare_strings(input, "")) {
                // Zero out the message buffer's ending.
                for (ssize i = message_length; i < input.size(); ++i) {
                    message_buffer[i] = '\0';
                }

                _ = cat::print("Recieved: ");
                _ = cat::print_line(message_buffer.p_data());
                break;
            }
        }
    }

    recieving_socket.close().or_panic();
    listening_socket.close().or_panic();
    _ = nix::sys_unlink(listening_socket.path_name.p_data()).or_panic();
    cat::exit();
}
