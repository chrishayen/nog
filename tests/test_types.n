fn test_int_equality() {
    int x = 42;
    int y = 42;
    assert_eq(x, y);
}

fn test_int_arithmetic() {
    int a = 10;
    int b = 5;
    assert_eq(a + b, 15);
    assert_eq(a - b, 5);
    assert_eq(a * b, 50);
    assert_eq(a / b, 2);
}

fn test_bool_equality() {
    bool t = true;
    bool f = false;
    assert_eq(t, true);
    assert_eq(f, false);
}

fn test_inferred_types() {
    x := 100;
    y := 100;
    assert_eq(x, y);
}

fn add(int a, int b) -> int {
    return a + b;
}

fn test_function_return() {
    int result = add(3, 4);
    assert_eq(result, 7);
}

fn test_f64_basic() {
    f64 x = 3.14;
    f64 y = 3.14;
    assert_eq(x, y);
}

fn test_f64_arithmetic() {
    f64 a = 10.0;
    f64 b = 2.5;
    assert_eq(a + b, 12.5);
    assert_eq(a - b, 7.5);
    assert_eq(a * b, 25.0);
    assert_eq(a / b, 4.0);
}

fn test_u32_basic() {
    u32 x = 42;
    u32 y = 42;
    assert_eq(x, y);
}

fn test_u32_arithmetic() {
    u32 a = 10;
    u32 b = 5;
    assert_eq(a + b, 15);
    assert_eq(a - b, 5);
    assert_eq(a * b, 50);
}

fn test_u64_basic() {
    u64 x = 100;
    u64 y = 100;
    assert_eq(x, y);
}

fn test_f32_basic() {
    f32 x = 3.14;
    f32 y = 3.14;
    assert_eq(x, y);
}

fn test_char_basic() {
    char c = 'a';
    char d = 'a';
    assert_eq(c, d);
}

fn test_char_different() {
    char x = 'x';
    char y = 'z';
    result := 0;

    if x != y {
        result = 1;
    }

    assert_eq(result, 1);
}
