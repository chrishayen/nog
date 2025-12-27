import tests.testlib;

fn test_qualified_call() {
    testlib.greet();
    assert_eq(1, 1);
}

fn test_qualified_call_with_return() {
    int result = testlib.add(2, 3);
    assert_eq(result, 5);
}
