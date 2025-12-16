fn test_parenthesized_grouping_changes_result() {
    assert_eq((1 + 2) * 3, 9);
}

fn test_parenthesized_nested_grouping() {
    assert_eq((1 + 2) * (3 + 4), 21);
}

fn test_parenthesized_expression_in_if() {
    x := 0;

    if (1 + 2) * 3 == 9 {
        x = 1;
    }

    assert_eq(x, 1);
}

fn test_parentheses_enable_postfix_on_expression() {
    assert_eq(("hi" + "there").length(), 7);
}

