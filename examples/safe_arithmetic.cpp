auto main() -> int {
    int myint = 0;
    u32 test_unsigned = 1u;

    // This is a compile error:
    // i64 data = test_unsigned;
    // i32 data = 10u;

    i64 data_2 = 10;
    // data_2 = 2.f;
    long foo = data_2;
    int foo2 = data_2;
    data_2 = foo2;
    i64 data_3;
    data_3 = 3l;
    data_3 = 4;
    data_3 = myint;
    i32 data_4 = 2;
    static_assert(sizeof(data_3) == 8);
    static_assert(sizeof(data_4) == 4);
    data_3 = data_4;  // Safe

    // This is a compile error:
    // data_4 = data_3;
}
