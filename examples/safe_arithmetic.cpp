auto meow() -> i64 {
    int myint = 0;
    u32 test_unsigned = 1u;

    // This is a compile error:
    // i64 data = test_unsigned;
    // i32 data = 10u;

    i64 data_2 = 10;
    // data_2 += 1;
    // static_assert()
    f32 data_f = 2.f;
    f32 data_f2 = f32(2.f);
    static_assert(sizeof(decltype(std::decay_integral(data_f))) == 4);
    data_f + data_f2;
    data_f2--;
    --data_f2;
    ++data_f2;
    data_f2++;
    long foo = data_2 - 1l;
    bool foobool = data_2 == 10;
    foo += 1;
    foo *= 2;
    int foo2 = data_2;
    data_2 = foo2;
    i64 data_3;
    data_3 = 3l;
    data_3 = 4;
    data_3 = myint;
    i32 data_4 = 2;
    auto data_5 = data_4 + data_2;
    static_assert(sizeof(data_3) == 8);
    static_assert(sizeof(data_4) == 4);
    data_3 = data_4;  // Safe

    // This is a compile error:
    // data_4 = data_3;
    return 0;
}
