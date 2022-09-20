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

    ssize len_1 = cat::string_length(p_string_1);
    ssize len_2 = cat::string_length(p_string_2);
    ssize len_3 = cat::string_length(p_string_3);
    ssize len_4 = cat::string_length(p_string_4);
    ssize len_5 = cat::string_length(p_string_5);
    ssize len_6 = cat::string_length(p_string_6);
    ssize len_7 = cat::string_length(p_string_7);

    verify(len_1 == len_2);
    verify(len_1 == 7);
    verify(len_3 == len_4);
    verify(len_5 == len_6);
    verify(len_7 == 15);

    // Test `String`s.
    cat::String string_1 = p_string_1;
    verify(string_1.size() == len_1);
    verify(string_1.subspan(1, 4).size() == 3);
    verify(string_1.first(4).size() == 4);
    verify(string_1.last(3).size() == 3);
    verify(cat::String("Hello!").size() == len_1);

    // TODO: Remove this and put it in another string unit test.
    char chars[5] = "foo\0";
    cat::Span<char> span = {chars, 4};
    span[0] = 'a';
    auto foo = cat::unconst(span).begin();
    *foo = 'a';
    for (char& c : span) {
        c = 'a';
    }

    // TODO: Put this in another string unit test.
    cat::String find_string = "abcdefabcdefabcdefabcdefabcdefabcdef";
    ssize c = find_string.find('c').or_exit();
    verify(c == 2);

    ssize a = find_string.find('a').or_exit();
    verify(a == 0);

    ssize f = find_string.find('f').or_exit();
    verify(f == 5);

    // `z` is not inside of a 32-byte chunk.
    cat::String find_string_2 =
        "abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcd"
        "efz";
    ssize z = find_string_2.find('z').or_exit();
    verify(z == 72);
}
