#include <cat/allocator>
#include <cat/format>
#include <cat/string>
#include <cat/utility>

auto main() -> int {
    char const* p_string_1 = "Hello!";
    char const* const p_string_2 = "Hello!";

    char const* const p_string_3 = "Hello!";
    char const* p_string_4 = "Hello!";

    char const* const p_string_5 = "Hello!";
    char const* const p_string_6 = "Hello!";

    char const* p_string_7 = "/tmp/temp.sock";

    iword len_1 = cat::string_length(p_string_1);
    iword len_2 = cat::string_length(p_string_2);
    iword len_3 = cat::string_length(p_string_3);
    iword len_4 = cat::string_length(p_string_4);
    iword len_5 = cat::string_length(p_string_5);
    iword len_6 = cat::string_length(p_string_6);
    iword len_7 = cat::string_length(p_string_7);

    cat::verify(len_1 == len_2);
    cat::verify(len_1 == 7);
    cat::verify(len_3 == len_4);
    cat::verify(len_5 == len_6);
    cat::verify(len_7 == 15);

    // Test `string`s.
    cat::string string_1 = p_string_1;
    cat::verify(string_1.size() == len_1);
    cat::verify(string_1.subspan(1, 4).size() == 3);
    cat::verify(string_1.first(4).size() == 4);
    cat::verify(string_1.last(3).size() == 3);
    cat::verify(cat::string("Hello!").size() == len_1);

    // TODO: Remove this and put it in another string unit test.
    char chars[5] = "foo\0";
    cat::span<char> span = {chars, 4};
    span[0] = 'a';
    auto foo = cat::unconst(span).begin();
    *foo = 'a';
    for (char& c : span) {
        c = 'a';
    }

    // TODO: Put this in another string unit test.
    cat::string find_string = "abcdefabcdefabcdefabcdefabcdefabcdef";
    iword c = find_string.find('c').or_exit();
    cat::verify(c == 2);

    iword a = find_string.find('a').or_exit();
    cat::verify(a == 0);

    iword f = find_string.find('f').or_exit();
    cat::verify(f == 5);

    // `z` is not inside of a 32-byte chunk.
    cat::string find_string_2 =
        "abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcd"
        "efz";
    iword z = find_string_2.find('z').or_exit();
    cat::verify(z == 72);
}
