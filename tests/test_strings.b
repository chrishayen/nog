fn test_string_equality() {
    assert_eq("hello", "hello");
}

fn test_another() {
    assert_eq("world", "world");
}

fn test_str_length() {
    s := "hello";
    assert_eq(s.length(), 5);
}

fn test_str_length_empty() {
    s := "";
    assert_eq(s.length(), 0);
}

fn test_str_empty_true() {
    s := "";
    assert_eq(s.empty(), true);
}

fn test_str_empty_false() {
    s := "hello";
    assert_eq(s.empty(), false);
}

fn test_str_contains_true() {
    s := "hello world";
    assert_eq(s.contains("world"), true);
}

fn test_str_contains_false() {
    s := "hello world";
    assert_eq(s.contains("foo"), false);
}

fn test_str_starts_with_true() {
    s := "hello world";
    assert_eq(s.starts_with("hello"), true);
}

fn test_str_starts_with_false() {
    s := "hello world";
    assert_eq(s.starts_with("world"), false);
}

fn test_str_ends_with_true() {
    s := "hello world";
    assert_eq(s.ends_with("world"), true);
}

fn test_str_ends_with_false() {
    s := "hello world";
    assert_eq(s.ends_with("hello"), false);
}

fn test_str_substr() {
    s := "hello world";
    assert_eq(s.substr(0, 5), "hello");
}

fn test_str_substr_middle() {
    s := "hello world";
    assert_eq(s.substr(6, 5), "world");
}

fn test_str_method_on_literal() {
    assert_eq("hello".length(), 5);
}

fn test_str_method_chaining() {
    s := "hello world";
    sub := s.substr(0, 5);
    assert_eq(sub.length(), 5);
}

fn test_str_concat() {
    s := "hello" + " " + "world";
    assert_eq(s, "hello world");
}

fn test_str_concat_variables() {
    a := "foo";
    b := "bar";
    c := a + b;
    assert_eq(c, "foobar");
}
