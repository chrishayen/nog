fn add(int a, int b) -> int {
    return a + b;
}

fn multiply(int a, int b) -> int {
    return a * b;
}

fn apply_op(int x, int y, fn(int, int) -> int op) -> int {
    return op(x, y);
}

fn test_function_ref() {
    result := apply_op(3, 4, add);
    assert_eq(result, 7);
}

fn test_function_ref_multiply() {
    result := apply_op(3, 4, multiply);
    assert_eq(result, 12);
}
